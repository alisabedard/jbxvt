/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury. */
//#undef DEBUG
#include "cmdtok.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "JBXVTCommandLimits.h"
#include "JBXVTToken.h"
#include "command.h"
#include "cursor.h"
#include "dcs.h"
#include "esc.h"
#include "libjb/log.h"
#include "libjb/xcb.h"
#include "utf.h"
#include "xevents.h"
// Flags used to control jbxvt_pop_char
enum ComCharFlags {INPUT_BUFFER_EMPTY = 0x100,
	GET_INPUT_ONLY=1, GET_XEVENTS_ONLY=2};
struct JBXVTCommandContainer { uint8_t *next, *top, *data; };
static struct JBXVTCommandContainer cmdtok_buffer, cmdtok_stack;
static void poll_io(xcb_connection_t * xc, fd_set * input_fdset)
{
	const fd_t xfd = xcb_get_file_descriptor(xc);
	const fd_t fd = jbxvt_get_fd();
	FD_ZERO(input_fdset);
	FD_SET(fd, input_fdset);
	FD_SET(xfd, input_fdset);
	if (select(fd + 1, input_fdset, NULL, NULL,
		&(struct timeval){.tv_usec = 500000}) == -1)
		exit(1); /* exit is reached in case SHELL or -e
			    command was not run successfully.  */

	/* If xfd has input, verify connection status.  Otherwise,
	   call timer function.  In this case, hook into the
	   cursor blink functionality.  FIXME:  Implement SGR blinking
	   text.  */
	(FD_ISSET(xfd, input_fdset) ? jb_check_x : jbxvt_blink_cursor)(xc);
}
static bool get_buffered(int16_t * val, const uint8_t flags)
{
	bool rval = true;
	if (cmdtok_stack.top > cmdtok_stack.data)
		*val = *--cmdtok_stack.top;
	else if (cmdtok_buffer.next < cmdtok_buffer.top)
		*val = *cmdtok_buffer.next++;
	else if (flags & GET_INPUT_ONLY)
		*val = INPUT_BUFFER_EMPTY;
	else
		rval = false;
	return rval;
}
/*  Return the next input character after first passing any
    keyboard input to the command.  If flags & GET_INPUT_ONLY
    is true then only buffered characters are returned and once
    the buffer is empty the special value INPUT_BUFFER_EMPTY
    is returned.  If flags and GET_XEVENTS_ONLY is true, then
    INPUT_BUFFER_EMPTY is returned when an X event arrives.
    This is the most often called function. */
int16_t jbxvt_pop_char(xcb_connection_t * xc, const uint8_t flags)
{
	int16_t ret = 0;
	if (!get_buffered(&ret, flags)) {
		xcb_flush(xc); // This is the primary xcb queue force
		fd_set in;
		bool go_on = true;
		do {
			FD_ZERO(&in);
			if (jbxvt_handle_xevents(xc)
				&& (flags & GET_XEVENTS_ONLY)) {
				go_on = false; // skip to return
				ret = INPUT_BUFFER_EMPTY;
			} else
				poll_io(xc, &in);
		} while (go_on && !FD_ISSET(jbxvt_get_fd(), &in));
		if (go_on) {
			const uint8_t l = read(jbxvt_get_fd(),
				cmdtok_buffer.data, COM_BUF_SIZE);
			if (l < 1)
				ret = errno == EWOULDBLOCK
					? INPUT_BUFFER_EMPTY : EOF;
			else {
				cmdtok_buffer.next = cmdtok_buffer.data;
				cmdtok_buffer.top = cmdtok_buffer.data + l;
				ret = *cmdtok_buffer.next++;
			}
		}
	}
	return ret;
}
/*  Return true if the character is one
    that can be handled by jbxvt_string() */
static inline bool is_string_char(register int16_t c)
{
	return c < 0x7f && (c >= ' ' || c == '\n'
		|| c == '\r' || c == '\t');
}
struct GetNewLinesData {
	xcb_connection_t * xc;
	uint8_t * s;
	int new_lines, c, i;
};
static void get_newlines(struct GetNewLinesData * restrict nld)
{
	do {
		if ((nld->s[nld->i++] = nld->c) == '\n')
			++nld->new_lines;
		nld->c = jbxvt_pop_char(nld->xc, GET_INPUT_ONLY);
	} while (is_string_char(nld->c)
		&& nld->i < JBXVT_TOKEN_MAX_LENGTH);
}
static void handle_string_char(xcb_connection_t * xc,
	int16_t c, struct JBXVTToken * restrict tk)
{
	tk->type = JBXVT_TOKEN_STRING;
	struct GetNewLinesData nld = {.xc = xc, .s = tk->string,
		.c =  c};
	get_newlines(&nld);
	tk->nlcount = nld.new_lines;
	tk->length = nld.i;
	c = nld.c;  // bring to this scope
	nld.s[nld.i] = 0; // terminating NULL
	if (c != INPUT_BUFFER_EMPTY)
		jbxvt_push_char(c);
}
static void default_token(xcb_connection_t * xc,
	struct JBXVTToken * restrict tk, int16_t c)
{
	switch(c) { // handle 8-bit controls
	case JBXVT_TOKEN_CSI: case JBXVT_TOKEN_DCS: case JBXVT_TOKEN_EPA:
	case JBXVT_TOKEN_HTS: case JBXVT_TOKEN_ID: case JBXVT_TOKEN_IND:
	case JBXVT_TOKEN_NEL: case JBXVT_TOKEN_OSC: case JBXVT_TOKEN_PM:
	case JBXVT_TOKEN_RI: case JBXVT_TOKEN_SOS: case JBXVT_TOKEN_SPA:
	case JBXVT_TOKEN_SS2: case JBXVT_TOKEN_SS3: case JBXVT_TOKEN_ST:
		tk->type = c;
		break;
	case JBXVT_TOKEN_APC: // Retrieve and skip sequence
#define POP() jbxvt_pop_char(xc, c)
		c = POP();
		c = POP();
		LOG("0x9f0x%x", (unsigned int)c);
		break;
	default:
		if (is_string_char(c)) {
			handle_string_char(xc, c, tk);
		} else {
			// Process individual characters and unicode:
			// additional bytes to parse:
			uint8_t bytes = jbxvt_get_utf_bytes(c);
			switch (bytes) {
			case 3:
#define UTFLOG(n) LOG("UTF8, %d more bytes: \t0x%x\n", n, (unsigned int)c)
				UTFLOG(3);
				jbxvt_parse_utf8_3(xc, tk, POP());
				break;
			case 2:
				UTFLOG(2);
				jbxvt_parse_utf8_2(xc, tk, POP());
				break;
			case 1:
				UTFLOG(1);
				jbxvt_parse_utf8_1(tk, POP());
				break;
			case 0:
				jbxvt_parse_utf8_0(tk, c);
				break;
			}
		}
	}
}
//  Return an input token
void jbxvt_get_token(xcb_connection_t * xc, struct JBXVTToken * restrict tk)
{
	memset(tk, 0, sizeof(struct JBXVTToken));
	const int16_t c = jbxvt_pop_char(xc, GET_XEVENTS_ONLY);
	switch (c) {
	case INPUT_BUFFER_EMPTY:
		tk->type = JBXVT_TOKEN_NULL;
		break;
	case EOF:
		tk->type = JBXVT_TOKEN_EOF;
		break;
	case JBXVT_TOKEN_ESC:
		jbxvt_esc(xc, c, tk);
		break;
	case JBXVT_TOKEN_CSI: // 8-bit CSI
		// Catch this here, since 7-bit CSI is parsed above.
		LOG("CC_CSI");
		jbxvt_csi(xc, c, tk);
		break;
	case JBXVT_TOKEN_DCS: // 8-bit DCS
		jbxvt_dcs(xc, tk);
		break;
	default:
		default_token(xc, tk, c);
	}
}
static void init_container(struct JBXVTCommandContainer * restrict c,
	uint8_t * restrict buf)
{
	c->data = c->next = c->top = buf;
}
void jbxvt_init_cmdtok(void)
{
	static uint8_t buf[COM_BUF_SIZE], stack[COM_PUSH_MAX];
	init_container(&cmdtok_buffer, buf);
	init_container(&cmdtok_stack, stack);
}
//  Push an input character back into the input queue.
void jbxvt_push_char(const uint8_t c)
{
	if (cmdtok_stack.top < cmdtok_stack.data + COM_PUSH_MAX)
		*cmdtok_stack.top++ = c;
}
