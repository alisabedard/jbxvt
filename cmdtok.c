/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "dcs.h"
#include "esc.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "sbar.h"
#include "scr_reset.h"
#include "xevents.h"
#include "window.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
enum {INPUT_BUFFER_EMPTY = 0x100};
//  Flags used to control jbxvt_pop_char();
enum ComCharFlags {GET_INPUT_ONLY=1, GET_XEVENTS_ONLY=2};
// Shortcuts
#define COM jbxvt.com
#define BUF jbxvt.com.buf
static void handle_focus(xcb_generic_event_t * restrict ge)
{
	xcb_focus_in_event_t * e = (xcb_focus_in_event_t *)ge;
	jbxvt.com.xev = (struct JBXVTEvent) {.type = e->response_type,
		.detail = e->detail};
	if (e->mode)
		  return;
	jbxvt.com.xev.detail = e->detail;
}
static void handle_sel(xcb_generic_event_t * restrict ge)
{
	xcb_selection_request_event_t * e
		= (xcb_selection_request_event_t *)ge;
	jbxvt.com.xev = (struct JBXVTEvent) {.type = e->response_type,
		.time = e->time, .requestor = e->requestor,
		.target = e->target, .property = e->property,
		.window = e->owner};
}
static void handle_client_msg(xcb_generic_event_t * restrict ge)
{
	xcb_client_message_event_t * e = (xcb_client_message_event_t *)ge;
	if (e->format == 32 && e->data.data32[0]
		== (unsigned long)jbxvt_get_wm_del_win())
		  exit(0);
}
static void handle_expose(xcb_generic_event_t * restrict ge)
{
	if (((xcb_expose_event_t *)ge)->window == jbxvt.X.win.sb)
		jbxvt_draw_scrollbar();
	else
		jbxvt_reset();
}
static void handle_other(xcb_generic_event_t * restrict ge)
{
	xcb_key_press_event_t * e = (xcb_key_press_event_t *)ge;
	jbxvt.com.xev = (struct JBXVTEvent) {.type = e->response_type,
		.window = e->event, .box.x = e->event_x,
		.box.y = e->event_y, .state = e->state,
		.time = e->time, .button = e->detail};
}
static void key_press(xcb_generic_event_t * restrict e)
{
	int_fast16_t count = 0;
	uint8_t * s = jbxvt_lookup_key(e, &count);
	write(jbxvt.com.fd, s, count);
}
static bool handle_xev(void)
{
	jb_check_x(jbxvt.X.xcb);
	xcb_generic_event_t * event = xcb_poll_for_event(jbxvt.X.xcb);
	if (!event)
		return false;
	switch (event->response_type & ~0x80) {
	case XCB_CONFIGURE_NOTIFY:
		jbxvt_resize_window();
		break;
	case XCB_KEY_PRESS:
		key_press(event);
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
		handle_client_msg(event);
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		handle_expose(event);
		break;
	default:
		handle_other(event);
	}
	free(event);
	return true;
}
static int_fast16_t output_to_command(void)
{
	struct JBXVTCommandData * c = &jbxvt.com;
	errno = 0;
	const ssize_t count = write(c->fd, c->send_nxt,
		c->send_count);
	jb_assert(count != -1, "Cannot write to command");
	c->send_count -= count;
	c->send_nxt += count;
	return count;
}
static void timer(void)
{
	if (!jbxvt.mode.att610 && jbxvt.opt.cursor_attr % 2) {
		jbxvt_draw_cursor(); // blinking cursor
		xcb_flush(jbxvt.X.xcb);
	}
}
__attribute__((nonnull))
static void poll_io(fd_set * restrict in_fdset)
{
	FD_SET(jbxvt.com.fd, in_fdset);
	FD_SET(jbxvt.com.xfd, in_fdset);
	fd_set out_fdset;
	FD_ZERO(&out_fdset);
	if (jbxvt.com.send_count > 0)
		FD_SET(jbxvt.com.fd, &out_fdset);
	errno = 0; // Ensure next error message is accurate
	if (select(jbxvt.com.width, in_fdset, &out_fdset, NULL,
		&(struct timeval){.tv_usec = 500000}) == -1)
		exit(0);
	if (FD_ISSET(jbxvt.com.fd, &out_fdset))
		output_to_command();
	else if (!FD_ISSET(jbxvt.com.xfd, in_fdset))
		timer(); // select timed out
	else
		jb_check_x(jbxvt.X.xcb);
}
static bool get_buffered(int_fast16_t * val, const uint8_t flags)
{
	if (COM.stack.top > COM.stack.data)
		*val = *--COM.stack.top;
	else if (COM.buf.next < COM.buf.top)
		*val = *COM.buf.next++;
	else if (flags & GET_INPUT_ONLY)
		*val = INPUT_BUFFER_EMPTY;
	else
		return false;
	return true;
}
/*  Return the next input character after first passing any keyboard input
    to the command.  If flags & GET_INPUT_ONLY is true then only buffered
    characters are returned and once the buffer is empty the special value
    INPUT_BUFFER_EMPTY is returned.  If flags and GET_XEVENTS_ONLY is true,
    then INPUT_BUFFER_EMPTY is returned when an X event arrives.
    This is the most often called function. */
int_fast16_t jbxvt_pop_char(const uint8_t flags)
{
	int_fast16_t ret = 0;
	if (get_buffered(&ret, flags))
		return ret;
	xcb_flush(jbxvt.X.xcb);
	fd_set in;
input:
	FD_ZERO(&in);
	if (handle_xev() && (flags & GET_XEVENTS_ONLY))
		return INPUT_BUFFER_EMPTY;
	poll_io(&in);
	if (!FD_ISSET(jbxvt.com.fd, &in))
		goto input;
	const uint8_t l = read(jbxvt.com.fd, BUF.data, COM_BUF_SIZE);
	if (l < 1)
		return errno == EWOULDBLOCK ? INPUT_BUFFER_EMPTY : EOF;
	BUF.next = BUF.data;
	BUF.top = BUF.data + l;
	return *BUF.next++;
}
//  Return true if the character is one that can be handled by jbxvt_string()
static inline bool is_string_char(register int_fast16_t c)
{
	return c < 0x7f && (c >= ' ' || c == '\n' || c == '\r' || c == '\t');
}
static void handle_string_char(int_fast16_t c, struct Token * restrict tk)
{
	uint_fast16_t i = 0, nl = 0;
	uint8_t * restrict s = tk->string;
	do {
		s[i++] = c;
		c = jbxvt_pop_char(GET_INPUT_ONLY);
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
static void handle_unicode(int_fast16_t c)
{
	LOG("handle_unicode(0x%x)", (unsigned int)c);
	c = jbxvt_pop_char(c);
	switch (c) {
	case 0x94:
		c = jbxvt_pop_char(c);
		switch (c) {
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
			LOG("0x%x", (unsigned int)c);
			jbxvt_push_char(c);
		}
		break;
	case 0x96:
	case 0x80:
		jbxvt_push_char('-');
		break;
	default:
		LOG("0xe2 0x%x", (unsigned int)c);
		jbxvt_push_char(c);
	}
}
static void default_token(struct Token * restrict tk, int_fast16_t c)
{
	switch(c) { // handle 8-bit controls
	case JBXVT_TOKEN_CSI: case JBXVT_TOKEN_DCS: case JBXVT_TOKEN_EPA: case JBXVT_TOKEN_HTS:
	case JBXVT_TOKEN_ID: case JBXVT_TOKEN_IND: case JBXVT_TOKEN_NEL: case JBXVT_TOKEN_OSC: case JBXVT_TOKEN_PM:
	case JBXVT_TOKEN_RI: case JBXVT_TOKEN_SOS: case JBXVT_TOKEN_SPA: case JBXVT_TOKEN_SS2: case JBXVT_TOKEN_SS3:
	case JBXVT_TOKEN_ST:
		tk->type = c;
		break;
	case JBXVT_TOKEN_APC: // Retrieve and skip sequence
		c = jbxvt_pop_char(c);
		c = jbxvt_pop_char(c);
		LOG("0x9f0x%x", (unsigned int)c);
		break;
	case 0xd0:
		LOG("Unknown character 0xd0");
		break;
	case 0xe2:
		LOG("0xe2");
		handle_unicode(c);
		break;
	default:
		if (is_string_char(c))
			handle_string_char(c, tk);
		else {
#ifdef CHAR_DEBUG
			LOG("0x%x", (unsigned int)c);
#endif//CHAR_DEBUG
			tk->type = JBXVT_TOKEN_CHAR;
			tk->tk_char = c;
		}
	}
}
//  Return an input token
void jbxvt_get_token(struct Token * restrict tk)
{
	memset(tk, 0, sizeof(struct Token));
	// set token per event:
	if(jbxvt_handle_xevents(&jbxvt.com.xev))
		  return;
	const int_fast16_t c = jbxvt_pop_char(GET_XEVENTS_ONLY);
	switch (c) {
	case INPUT_BUFFER_EMPTY:
		tk->type = JBXVT_TOKEN_NULL;
		break;
	case EOF:
		tk->type = JBXVT_TOKEN_EOF;
		break;
	case JBXVT_TOKEN_ESC:
		jbxvt_esc(c, tk);
		break;
	case JBXVT_TOKEN_CSI: // 8-bit CSI
		// Catch this here, since 7-bit CSI is parsed above.
		LOG("CC_CSI");
		jbxvt_csi(c, tk);
		break;
	case JBXVT_TOKEN_DCS: // 8-bit DCS
		jbxvt_dcs(tk);
		break;
	default:
		default_token(tk, c);
	}
}
