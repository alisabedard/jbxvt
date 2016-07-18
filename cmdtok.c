/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cmdtok.h"

#include "jbxvt.h"
#include "lookup_key.h"
#include "ttyinit.h"
#include "wm_del_win.h"
#include "xevents.h"
#include "xeventst.h"

#include <errno.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static JBXVTEvent * ev_alloc(xcb_generic_event_t * restrict e)
{
	JBXVTEvent * xe = GC_MALLOC(sizeof(JBXVTEvent));
	xe->type = e->response_type;
	return xe;
}

static void handle_focus(xcb_generic_event_t * restrict e)
{
	xcb_focus_in_event_t * f = (xcb_focus_in_event_t *)e;
	if (f->mode)
		  return;
	switch (f->detail) {
	case XCB_NOTIFY_DETAIL_ANCESTOR:
	case XCB_NOTIFY_DETAIL_INFERIOR:
	case XCB_NOTIFY_DETAIL_NONLINEAR:
		break;
	default:
		return;
	}
	JBXVTEvent * xe = ev_alloc(e);
	xe->detail = f->detail;
	push_xevent(xe);
}

static void handle_sel(xcb_generic_event_t * restrict ge)
{
	JBXVTEvent * xe = ev_alloc(ge);
	xcb_selection_request_event_t * e
		= (xcb_selection_request_event_t *)ge;
	xe->time = e->time;
	xe->requestor = e->requestor;
	xe->target = e->target;
	xe->property = e->property;
	xe->window = e->owner;
	push_xevent(xe);
}

static void handle_client_msg(xcb_generic_event_t * restrict ge)
{
	xcb_client_message_event_t * e = (xcb_client_message_event_t *)ge;
	if (e->format == 32 && e->data.data32[0]
		== (long)wm_del_win())
		  exit(0);
}

static void handle_expose(xcb_generic_event_t * restrict ge)
{
	xcb_expose_event_t * e = (xcb_expose_event_t *)ge;
	JBXVTEvent * xe = ev_alloc(ge);
	xe->window = e->window;
	xe->box.x = e->x;
	xe->box.y = e->y;
	xe->box.width = e->width;
	xe->box.height = e->height;
	push_xevent(xe);
}

static void handle_other(xcb_generic_event_t * restrict ge)
{
	xcb_key_press_event_t * e = (xcb_key_press_event_t *)ge;
	JBXVTEvent * xe = ev_alloc(ge);
	xe->window = e->event;
	xe->box.x = e->event_x;
	xe->box.y = e->event_y;
	xe->state = e->state;
	xe->button = e->detail;
	xe->time = e->time;
	push_xevent(xe);
}

static int_fast16_t handle_xev(xcb_generic_event_t * restrict event,
	int_fast16_t * restrict count, const uint8_t flags)
{
	uint8_t * s;

	switch (event->response_type & ~0x80) {
	case XCB_KEY_PRESS:
		s = lookup_key(event, count);
		if (count) {
			jbxvt.com.send_nxt = s;
			jbxvt.com.send_count = *count;
		}
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
	if (flags & GET_XEVENTS)
		  return GCC_NULL;
	return 0;
}

static int_fast16_t output_to_command(void)
{
	const ssize_t count = write(jbxvt.com.fd, jbxvt.com.send_nxt,
		jbxvt.com.send_count);
	if (jb_check(count != -1, "Could not write to command"))
		exit(1);
	jbxvt.com.send_count -= count;
	jbxvt.com.send_nxt += count;
	return count;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((regparm(1)))
#endif//x86
static int_fast16_t poll_io(int_fast16_t count, fd_set * restrict in_fdset)
{
	const fd_t x_fd = xcb_get_file_descriptor(jbxvt.X.xcb);
	FD_SET(jbxvt.com.fd, in_fdset);
	FD_SET(x_fd, in_fdset);
	fd_set out_fdset;
	FD_ZERO(&out_fdset);
	if (jbxvt.com.send_count > 0)
		  FD_SET(jbxvt.com.fd, &out_fdset);
	int sv;
	sv = select(jbxvt.com.width, in_fdset,&out_fdset, NULL, NULL);
	if (sv != -1 && FD_ISSET(jbxvt.com.fd, &out_fdset)) {
		count = output_to_command();
	}
	return count;
}

/*  Return the next input character after first passing any keyboard input
 *  to the command.  If flags & BUF_ONLY is true then only buffered characters are
 *  returned and once the buffer is empty the special value GCC_NULL is
 *  returned.  If flags and GET_XEVENTS is true then GCC_NULL is returned
 *  when an X event arrives.
 */
// This is the most often called function.
#if defined(__i386__) || defined(__amd64__)
__attribute__((hot,regparm(1)))
#else
__attribute__((hot))
#endif
static int_fast16_t get_com_char(const int_fast8_t flags)
{
	if (jbxvt.com.stack.top > jbxvt.com.stack.data)
		return(*--jbxvt.com.stack.top);

	if (jbxvt.com.buf.next < jbxvt.com.buf.top)
		return(*jbxvt.com.buf.next++);

	if (flags & BUF_ONLY)
		return(GCC_NULL);
	// Flush here to draw the cursor.
	xcb_flush(jbxvt.X.xcb);
	int_fast16_t count = 0;
	fd_set in_fdset;
	do {
		FD_ZERO(&in_fdset);
		xcb_generic_event_t * e;
		if ((e = xcb_poll_for_event(jbxvt.X.xcb))) {
			const int_fast16_t xev_ret
				= handle_xev(e, &count, flags);
			free(e);
			if (xev_ret)
				  return xev_ret;
		}
		count = poll_io(count, &in_fdset);
	} while(!FD_ISSET(jbxvt.com.fd,&in_fdset));
	uint8_t * d = jbxvt.com.buf.data;
	count = read(jbxvt.com.fd, d, COM_BUF_SIZE);
	if (count < 1) // buffer is empty
		return errno == EWOULDBLOCK ? GCC_NULL : EOF;
	jbxvt.com.buf.next = d;
	jbxvt.com.buf.top = d + count;
	return *jbxvt.com.buf.next++;
}

//  Return true if the character is one that can be handled by scr_string()
#if defined(__i386__) || defined(__amd64__)
	__attribute__((hot,const,regparm(1)))
#else
	__attribute__((hot,const))
#endif//x86
static inline bool is_string_char(register int_fast16_t c)
{
	c &= 0177;
	return c >= ' ' || c == '\n' || c == '\r' || c == '\t';
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void handle_string_char(int_fast16_t c, Token * restrict tk)
{
	uint_fast16_t i = 0;
	tk->nlcount = 0;
	do {
		tk->string[i++] = c;
		c = get_com_char(1);
		if (c == '\n' && ++tk->nlcount >= NLMAX) {
			--tk->nlcount;
			break;
		}
	} while (is_string_char(c) && i < TKS_MAX);
	tk->length = i;
	tk->string[i] = 0;
	tk->type = TK_STRING;
	if (c != GCC_NULL)
		  push_com_char(c);
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void start_esc(int_fast16_t c, Token * restrict tk)
{
	c = get_com_char(0);
	if (c >= '<' && c <= '?') {
		tk->private = c;
		c = get_com_char(0);
	}

	//  read any numerical arguments
	uint_fast16_t i = 0;
	do {
		uint_fast16_t n = 0;
		while (c >= '0' && c <= '9') {
			n = n * 10 + c - '0';
			c = get_com_char(0);
		}
		if (i < TK_MAX_ARGS)
			  tk->arg[i++] = n;
		if (c == ESC)
			  push_com_char(c);
		if (c < ' ')
			  return;
		if (c < '@')
			  c = get_com_char(0);
	} while (c < '@' && c >= ' ');
	if (c == ESC)
		  push_com_char(c);
	tk->nargs = i;
	tk->type = c;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void end_esc(int_fast16_t c, Token * restrict tk)
{
	c = get_com_char(0);
	uint_fast16_t n = 0;
	while (c >= '0' && c <= '9') {
		n = n * 10 + c - '0';
		c = get_com_char(0);
	}
	tk->arg[0] = n;
	tk->nargs = 1;
	c = get_com_char(0);
	register uint_fast16_t i = 0;
	while ((c & 0177) >= ' ' && i < TKS_MAX) {
		if (c >= ' ')
			  tk->string[i++] = c;
		c = get_com_char(0);
	}
	tk->length = i;
	tk->string[i] = 0;
	tk->type = TK_TXTPAR;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void handle_esc(int_fast16_t c, Token * restrict tk)
{
	c = get_com_char(0);
	switch(c) {
	case '[': // CSI
		start_esc(c, tk);
		break;
	case ']': // OSC
		end_esc(c, tk);
		break;
	case '#': // DECSWH, or prelude to DECALN
	case '(': // G0 charset
	case ')': // G1 charset
		tk->type = c;
		c = get_com_char(0);
		tk->arg[0] = c;
		tk->nargs = 1;
		break;
	case '7': // DECSC: save cursor
	case '8': // DECRC: restore cursor
	case '=': // DECPAM: keypad to application mode
	case '>': // DECPNM: keypad to numeric mode
	case '^': // DECPM: Privacy message (ended by ESC \)
	case '\\':
		tk->type = c;
		tk->nargs = 0;
		break;
	case 'c': // Reset to Initial State
		tk->type = TK_RIS;
		break;
	case 'D' :
		tk->type = TK_IND;
		break;
	case 'E' :
		tk->type = TK_NEL;
		break;
	case 'F': // Enter VT52 graphics mode
		tk->type = TK_ENTGM52;
		break;
	case 'G': // Leave VT52 graphics mode
		tk->type = TK_EXTGM52;
		break;
	case 'H' :
		tk->type = TK_HTS;
		break;
	case 'M' :
		tk->type = TK_RI;
		break;
	case 'N' :
		tk->type = TK_SS2;
		break;
	case 'O' :
		tk->type = TK_SS3;
		break;
	case 'Z' :
		tk->type = TK_DECID;
		break;
	}
}

//  Return an input token
void get_token(Token * restrict tk)
{
	memset(tk, 0, sizeof(Token));
	// set token per event:
	if(handle_xevents(tk))
		  return;
	const int_fast16_t c = get_com_char(GET_XEVENTS);
	switch (c) {
	case GCC_NULL:
		tk->type = TK_NULL;
		break;
	case EOF:
		tk->type = TK_EOF;
		break;
	case ESC:
		handle_esc(c, tk);
		break;
	default:
		if (is_string_char(c))
			  handle_string_char(c, tk);
		else {
			tk->type = TK_CHAR;
			tk->tk_char = c;
		}
	}
}

