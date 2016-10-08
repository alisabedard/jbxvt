/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cmdtok.h"

#include "cursor.h"
#include "dcs.h"
#include "esc.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "xevents.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

enum {INPUT_BUFFER_EMPTY = 0x100};

//  Flags used to control get_com_char();
enum ComCharFlags {GET_INPUT_ONLY=1, GET_XEVENTS_ONLY=2};

// Shortcuts
#define XC jbxvt.X.xcb
#define COM jbxvt.com
#define BUF jbxvt.com.buf
#define CFD COM.fd

#ifdef DEBUG
static void print_queued_event_count(void)
{
	uint16_t i = 0;
	for (struct JBXVTEvent * e = jbxvt.com.events.start; e; e = e->next)
		++i;
	if (i)
		LOG("%u events queued", i);
}
#else//!DEBUG
#define print_queued_event_count()
#endif//DEBUG

static struct JBXVTEvent * ev_alloc(xcb_generic_event_t * restrict e)
{
	struct JBXVTEvent * xe = calloc(1, sizeof(struct JBXVTEvent));
	xe->type = e->response_type;
	return xe;
}

static void put_xevent(struct JBXVTEvent * xe)
{
	struct JBXVTEventQueue * q = &jbxvt.com.events;
	xe->next = q->start;
	xe->prev = NULL;
	*(xe->next ? &xe->next->prev : &q->last) = xe;
	print_queued_event_count();
}

static void handle_focus(xcb_generic_event_t * restrict e)
{
	xcb_focus_in_event_t * f = (xcb_focus_in_event_t *)e;
	if (f->mode)
		  return;
	struct JBXVTEvent * xe = ev_alloc(e);
	xe->detail = f->detail;
	put_xevent(xe);
}

#define XESET(a, b) xe->a = e->b
#define XEEQ(a) XESET(a, a)
#define EALLOC(t) struct JBXVTEvent * xe = ev_alloc(ge); t * e = (t*)ge;

static void handle_sel(xcb_generic_event_t * restrict ge)
{
	EALLOC(xcb_selection_request_event_t);
	XEEQ(time); XEEQ(requestor); XEEQ(target); XEEQ(property);
	XESET(window, owner);
	put_xevent(xe);
}

static void handle_client_msg(xcb_generic_event_t * restrict ge)
{
	xcb_client_message_event_t * e = (xcb_client_message_event_t *)ge;
	if (e->format == 32 && e->data.data32[0]
		== (unsigned long)wm_del_win())
		  exit(0);
}

static void handle_expose(xcb_generic_event_t * restrict ge)
{
	EALLOC(xcb_expose_event_t);
	XEEQ(window); XESET(box.x, x); XESET(box.y, y);
	XESET(box.width, width); XESET(box.height, height);
	put_xevent(xe);
}

static void handle_other(xcb_generic_event_t * restrict ge)
{
	EALLOC(xcb_key_press_event_t);
	XESET(window, event); XESET(box.x, event_x); XESET(box.y, event_y);
	XEEQ(state); XEEQ(time); XESET(button, detail);
	put_xevent(xe);
}

static void key_press(xcb_generic_event_t * restrict e)
{
	int_fast16_t count = 0;
	uint8_t * s = lookup_key(e, &count);
	if (count) {
		jbxvt.com.send_nxt = s;
		jbxvt.com.send_count = count;
	}
}

static bool handle_xev(void)
{
	jb_check_x(XC);
	xcb_generic_event_t * event = xcb_poll_for_event(XC);
	if (!event)
		return false;
	switch (event->response_type & ~0x80) {
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
	if (jbxvt.mode.att610)
		return;
	switch(jbxvt.opt.cursor_attr) {
	case 0: // blinking block
	case 1: // blinking block
	case 3: // blinking underline
	case 5: // blinking bar
	case 7: // blinking overline
		draw_cursor();
		xcb_flush(jbxvt.X.xcb); // enable clearing
		break;
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
	jb_assert(select(jbxvt.com.width, in_fdset, &out_fdset, NULL,
		&(struct timeval){.tv_usec = 500000}) != -1,
		"Select failed.");
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
int_fast16_t get_com_char(const uint8_t flags)
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
	if (!FD_ISSET(CFD, &in))
		goto input;
	const uint8_t l = read(CFD, BUF.data, COM_BUF_SIZE);
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
		c = get_com_char(GET_INPUT_ONLY);
		if (c == '\n')
			++nl;
	} while (is_string_char(c) && i < TKS_MAX);
	tk->nlcount = nl;
	tk->length = i;
	s[i] = 0; // terminating NULL
	tk->type = TK_STRING;
	if (c != INPUT_BUFFER_EMPTY)
		  put_com_char(c);
}

static void handle_unicode(int_fast16_t c)
{
	LOG("handle_unicode(0x%x)", (unsigned int)c);
	c = get_com_char(c);
	switch (c) {
	case 0x94:
		c = get_com_char(c);
		switch (c) {
		case 0x80:
			put_com_char('-');
			break;
		case 0x82:
			put_com_char('|');
			break;
		case 0xac:
			put_com_char('+');
			break;
		default:
			LOG("0x%x", (unsigned int)c);
			put_com_char(c);
		}
		break;
	case 0x96:
	case 0x80:
		put_com_char('-');
		break;
	default:
		LOG("0xe2 0x%x", (unsigned int)c);
		put_com_char(c);
	}
}


static void default_token(struct Token * restrict tk, int_fast16_t c)
{

	switch(c) { // handle 8-bit controls
	case TK_CSI: case TK_DCS: case TK_EPA: case TK_HTS:
	case TK_ID: case TK_IND: case TK_NEL: case TK_OSC: case TK_PM:
	case TK_RI: case TK_SOS: case TK_SPA: case TK_SS2: case TK_SS3:
	case TK_ST:
		tk->type = c;
		break;
	case TK_APC: // Retrieve and skip sequence
		c = get_com_char(c);
		c = get_com_char(c);
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
			tk->type = TK_CHAR;
			tk->tk_char = c;
		}
	}
}

//  Return an input token
void get_token(struct Token * restrict tk)
{
	memset(tk, 0, sizeof(struct Token));
	// set token per event:
	if(handle_xevents(tk))
		  return;
	const int_fast16_t c = get_com_char(GET_XEVENTS_ONLY);
	switch (c) {
	case INPUT_BUFFER_EMPTY:
		tk->type = TK_NULL;
		break;
	case EOF:
		tk->type = TK_EOF;
		break;
	case TK_ESC:
		jbxvt_esc(c, tk);
		break;
	case TK_CSI: // 8-bit CSI
		// Catch this here, since 7-bit CSI is parsed above.
		LOG("CC_CSI");
		jbxvt_csi(c, tk);
		break;
	case TK_DCS: // 8-bit DCS
		jbxvt_dcs(tk);
		break;
	default:
		default_token(tk, c);
	}
}

