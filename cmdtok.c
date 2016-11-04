/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "dcs.h"
#include "esc.h"
#include "libjb/log.h"
#include "libjb/xcb.h"
#include "lookup_key.h"
#include "mode.h"
#include "sbar.h"
#include "scr_reset.h"
#include "window.h"
#include "xevents.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
//  Flags used to control jbxvt_pop_char
enum ComCharFlags {INPUT_BUFFER_EMPTY = 0x100,
	GET_INPUT_ONLY=1, GET_XEVENTS_ONLY=2};
struct JBXVTCommandContainer {
	uint8_t *next, *top, *data;
};
static struct JBXVTEvent cmdtok_xev;
static struct JBXVTCommandContainer cmdtok_buffer;
static struct JBXVTCommandContainer cmdtok_stack;
// Shortcuts
//#define BUF cmdtok_buffer
static void handle_focus(xcb_generic_event_t * restrict ge)
{
	xcb_focus_in_event_t * e = (xcb_focus_in_event_t *)ge;
	cmdtok_xev = (struct JBXVTEvent) {.type = e->response_type,
		.detail = e->detail};
	if (e->mode)
		  return;
	cmdtok_xev.detail = e->detail;
}
static void handle_sel(xcb_generic_event_t * restrict ge)
{
	xcb_selection_request_event_t * e
		= (xcb_selection_request_event_t *)ge;
	cmdtok_xev = (struct JBXVTEvent) {.type = e->response_type,
		.time = e->time, .requestor = e->requestor,
		.target = e->target, .property = e->property,
		.window = e->owner};
}
static void handle_client_msg(xcb_connection_t * xc,
	xcb_generic_event_t * restrict ge)
{
	xcb_client_message_event_t * e = (xcb_client_message_event_t *)ge;
	if (e->format == 32 && e->data.data32[0]
		== (unsigned long)jbxvt_get_wm_del_win(xc))
		  exit(0);
}
static void handle_expose(xcb_connection_t * xc,
	xcb_generic_event_t * restrict ge)
{
	if (((xcb_expose_event_t *)ge)->window == jbxvt_get_scrollbar(xc))
		jbxvt_draw_scrollbar(xc);
	else
		jbxvt_reset(xc);
}
static void handle_other(xcb_generic_event_t * restrict ge)
{
	xcb_key_press_event_t * e = (xcb_key_press_event_t *)ge;
	cmdtok_xev = (struct JBXVTEvent) {.type = e->response_type,
		.window = e->event, .box.x = e->event_x,
		.box.y = e->event_y, .state = e->state,
		.time = e->time, .button = e->detail};
}
static void key_press(xcb_connection_t * xc,
	xcb_generic_event_t * restrict e)
{
	int_fast16_t count = 0;
	uint8_t * s = jbxvt_lookup_key(xc, e, &count);
	jb_require(write(jbxvt_get_fd(), s, count) != -1,
		"Could not write to command");
}
static bool handle_xev(xcb_connection_t * xc)
{
	jb_check_x(xc);
	xcb_generic_event_t * event = xcb_poll_for_event(xc);
	if (!event)
		return false;
	switch (event->response_type & ~0x80) {
	case XCB_CONFIGURE_NOTIFY:
		jbxvt_resize_window(xc);
		break;
	case XCB_KEY_PRESS:
		key_press(xc, event);
		break;
	case XCB_FOCUS_IN:
	case XCB_FOCUS_OUT:
		handle_focus(event);
		break;
	case XCB_SELECTION_REQUEST:
	case XCB_SELECTION_NOTIFY:
		handle_sel(event);
		break;
	case XCB_CLIENT_MESSAGE:
		handle_client_msg(xc, event);
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		handle_expose(xc, event);
		break;
	default:
		handle_other(event);
	}
	free(event);
	return true;
}
static void timer(xcb_connection_t * xc)
{
	jbxvt_blink_cursor(xc);
}
static void check_fdsets(xcb_connection_t * xc,
	fd_set * restrict in_fdset, const fd_t xfd)
{
	if (!FD_ISSET(xfd, in_fdset))
		timer(xc); // select timed out
	else
		jb_check_x(xc);
}
__attribute__((nonnull))
static void poll_io(xcb_connection_t * xc,
	fd_set * restrict in_fdset)
{
	static fd_t fd, xfd;
	static int nfds; // per man select(2)
	if (!fd)
		fd = jbxvt_get_fd();
	if (!xfd)
		xfd = xcb_get_file_descriptor(xc);
	if (!nfds)
		nfds = fd + 1;
#ifdef DEBUG_FDS
	// nfds should be highest plus one
	LOG("fd: %d, %d, %d\n", fd, xfd, nfds);
#endif//DEBUG_FDS
	FD_SET(fd, in_fdset);
	FD_SET(xfd, in_fdset);
	fd_set out_fdset;
	FD_ZERO(&out_fdset);
	if (select(nfds, in_fdset, &out_fdset, NULL,
		&(struct timeval){.tv_usec = 500000}) == -1)
		exit(1); /* exit is reached in case SHELL or -e
			    command was not run successfully.  */
	check_fdsets(xc, in_fdset, xfd);
}
static bool get_buffered(int_fast16_t * val, const uint8_t flags)
{
	if (cmdtok_stack.top > cmdtok_stack.data)
		*val = *--cmdtok_stack.top;
	else if (cmdtok_buffer.next < cmdtok_buffer.top)
		*val = *cmdtok_buffer.next++;
	else if (flags & GET_INPUT_ONLY)
		*val = INPUT_BUFFER_EMPTY;
	else
		return false;
	return true;
}
/*  Return the next input character after first passing any
    keyboard input to the command.  If flags & GET_INPUT_ONLY
    is true then only buffered characters are returned and once
    the buffer is empty the special value INPUT_BUFFER_EMPTY
    is returned.  If flags and GET_XEVENTS_ONLY is true, then
    INPUT_BUFFER_EMPTY is returned when an X event arrives.
    This is the most often called function. */
int_fast16_t jbxvt_pop_char(xcb_connection_t * xc, const uint8_t flags)
{
	int_fast16_t ret = 0;
	if (get_buffered(&ret, flags))
		return ret;
	xcb_flush(xc);
	fd_set in;
	do {
		FD_ZERO(&in);
		if (handle_xev(xc) && (flags & GET_XEVENTS_ONLY))
			return INPUT_BUFFER_EMPTY;
		poll_io(xc, &in);
	} while (!FD_ISSET(jbxvt_get_fd(), &in));
	const uint8_t l = read(jbxvt_get_fd(),
		cmdtok_buffer.data, COM_BUF_SIZE);
	if (l < 1)
		return errno == EWOULDBLOCK ? INPUT_BUFFER_EMPTY : EOF;
	cmdtok_buffer.next = cmdtok_buffer.data;
	cmdtok_buffer.top = cmdtok_buffer.data + l;
	return *cmdtok_buffer.next++;
}
/*  Return true if the character is one
    that can be handled by jbxvt_string() */
static inline bool is_string_char(register int_fast16_t c)
{
	return c < 0x7f && (c >= ' ' || c == '\n'
		|| c == '\r' || c == '\t');
}
static void handle_string_char(xcb_connection_t * xc,
	int_fast16_t c, struct Token * restrict tk)
{
	uint_fast16_t i = 0, nl = 0;
	uint8_t * restrict s = tk->string;
	do {
		s[i++] = c;
		c = jbxvt_pop_char(xc, GET_INPUT_ONLY);
		if (c == '\n')
			++nl;
	} while (is_string_char(c) && i < JBXVT_TOKEN_MAX_LENGTH);
	tk->nlcount = nl;
	tk->length = i;
	s[i] = 0; // terminating NULL
	tk->type = JBXVT_TOKEN_STRING;
	if (c != INPUT_BUFFER_EMPTY)
		  jbxvt_push_char(c);
}
__attribute__((const))
static uint8_t get_utf_bytes(uint8_t c)
{
	if ((c & 0xf0) == 0xf0)
		return 3;
	if ((c & 0xe0) == 0xe0)
		return 2;
	if ((c & 0xc0) == 0xc0)
		return 1;
	return 0;
}
static void utf8_3(xcb_connection_t * xc,
	struct Token * restrict tk, int_fast16_t c) // 1
{
	LOG("utf8_3()");
	LOG("\t0x%x\n", (unsigned int)c);
	c = jbxvt_pop_char(xc, c); // 2
	LOG("\t0x%x\n", (unsigned int)c);
	c = jbxvt_pop_char(xc, c); // 3
	LOG("\t0x%x\n", (unsigned int)c);
	switch (c) {
	default:
		tk->type = JBXVT_TOKEN_NULL;
	}
}
static void utf8_2(xcb_connection_t * xc,
	struct Token * restrict tk, int_fast16_t c) // 1
{
	LOG("utf8_2()");
	LOG("\t0x%x\n", (unsigned int)c);
	int_fast16_t c2 = jbxvt_pop_char(xc, c); // take next byte
	LOG("\t0x%x\n", (unsigned int)c2);
	switch (c) {
	case 0x80:
		switch (c2) {
		case 0x90:
			jbxvt_push_char('-');
			break;
		default:
			goto tk_null;
		}
		break;
	case 0x94:
		switch (c2) {
		case 0x80:
			jbxvt_push_char('-');
			break;
		case 0x82:
			jbxvt_push_char('|');
			break;
		case 0xac:
			jbxvt_push_char('+');
			break;
		default:
			goto tk_null;
		}
		break;
	case 0x96:
		switch (c2) {
		case 0xbd: // FIXME:  white down pointing triangle
			jbxvt_push_char('V');
			break;
		default:
			goto tk_null;
		}
	default:
tk_null:
		tk->type = JBXVT_TOKEN_NULL;
	}
}
static void utf8_1(struct Token * restrict tk, int_fast16_t c) // 1
{
	LOG("utf8_1()");
	LOG("\t0x%x\n", (unsigned int)c);
	switch (c) {
	default:
		tk->type = JBXVT_TOKEN_NULL;
	}
}
static void utf8_0(struct Token * restrict tk, int_fast16_t c)
{
	tk->type = JBXVT_TOKEN_CHAR;
	tk->tk_char = c;
}
static void default_token(xcb_connection_t * xc,
	struct Token * restrict tk, int_fast16_t c)
{
	switch(c) { // handle 8-bit controls
	case JBXVT_TOKEN_CSI: case JBXVT_TOKEN_DCS: case JBXVT_TOKEN_EPA:
	case JBXVT_TOKEN_HTS: case JBXVT_TOKEN_ID: case JBXVT_TOKEN_IND:
	case JBXVT_TOKEN_NEL: case JBXVT_TOKEN_OSC: case JBXVT_TOKEN_PM:
	case JBXVT_TOKEN_RI: case JBXVT_TOKEN_SOS: case JBXVT_TOKEN_SPA:
	case JBXVT_TOKEN_SS2: case JBXVT_TOKEN_SS3: case JBXVT_TOKEN_ST:
		tk->type = c;
		return;
	case JBXVT_TOKEN_APC: // Retrieve and skip sequence
		c = jbxvt_pop_char(xc, c);
		c = jbxvt_pop_char(xc, c);
		LOG("0x9f0x%x", (unsigned int)c);
		return;
	}
	if (is_string_char(c)) {
		handle_string_char(xc, c, tk);
		return;
	}
	// Process individual characters and unicode:
	uint8_t bytes = get_utf_bytes(c); // additional bytes to parse
	switch (bytes) {
	case 3:
		LOG("UTF8, 3 additional bytes: \t0x%x\n", (unsigned int)c);
		utf8_3(xc, tk, jbxvt_pop_char(xc, c));
		break;
	case 2:
		LOG("UTF8, 2 additional bytes: \t0x%x\n", (unsigned int)c);
		utf8_2(xc, tk, jbxvt_pop_char(xc, c));
		break;
	case 1:
		LOG("UTF8, 1 additional byte: \t0x%x\n", (unsigned int)c);
		utf8_1(tk, jbxvt_pop_char(xc, c));
		break;
	case 0:
		utf8_0(tk, c);
		break;
	}
}
//  Return an input token
void jbxvt_get_token(xcb_connection_t * xc, struct Token * restrict tk)
{
	memset(tk, 0, sizeof(struct Token));
	// set token per event:
	if(jbxvt_handle_xevents(xc, &cmdtok_xev)) {
		// Zero out event structure for next event:
		cmdtok_xev = (struct JBXVTEvent){};
		return;
	}
	const int_fast16_t c = jbxvt_pop_char(xc, GET_XEVENTS_ONLY);
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
const char * jbxvt_get_csi(void)
{
	return jbxvt_get_modes()->s8c1t ? "\233" : "\033[";
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
