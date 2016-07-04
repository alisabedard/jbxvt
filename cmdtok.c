/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cmdtok.h"

#include "command.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "scr_string.h"
#include "token.h"
#include "ttyinit.h"
#include "wm_del_win.h"
#include "xevents.h"
#include "xeventst.h"
#include "xsetup.h"

#include <errno.h>
#include <gc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>

static void handle_focus(xcb_focus_in_event_t * restrict e)
{
	if (e->mode)
		  return;
	switch (e->detail) {
	case XCB_NOTIFY_DETAIL_ANCESTOR:
	case XCB_NOTIFY_DETAIL_INFERIOR:
	case XCB_NOTIFY_DETAIL_NONLINEAR:
		break;
	default:
		return;
	}
	struct xeventst * xe = GC_MALLOC(sizeof(struct xeventst));
	xe->xe_type = e->response_type & ~0x80;
	xe->xe_detail = e->detail;
	push_xevent(xe);
}

static void handle_sel_req(xcb_selection_request_event_t * restrict e)
{
	struct xeventst * xe = GC_MALLOC(sizeof(struct xeventst));
	xe->xe_type = XCB_SELECTION_REQUEST;
	xe->xe_window = e->owner;
	xe->xe_time = e->time;
	xe->xe_requestor = e->requestor;
	xe->xe_target = e->target;
	xe->xe_property = e->property;
	push_xevent(xe);
}

static void handle_sel_not(xcb_selection_notify_event_t * restrict e)
{
	struct xeventst * xe = GC_MALLOC(sizeof(struct xeventst));
	xe->xe_type = XCB_SELECTION_NOTIFY;
	xe->xe_time = e->time;
	xe->xe_requestor = e->requestor;
	xe->xe_target = e->target;
	xe->xe_property = e->property;
	push_xevent(xe);
}

static void handle_client_msg(xcb_client_message_event_t * e)
{
	if (e->format == 32 && e->data.data32[0]
		== (long)wm_del_win())
		  quit(0, NULL);
}

static void handle_expose(xcb_expose_event_t * e)
{
	struct xeventst * xe = GC_MALLOC(sizeof(struct xeventst));
	xe->xe_type = XCB_EXPOSE;
	xe->xe_window = e->window;
	xe->xe_x = e->x;
	xe->xe_y = e->y;
	xe->xe_width = e->width;
	xe->xe_height = e->height;
	push_xevent(xe);
}

static void handle_other(xcb_generic_event_t * gen_e)
{
	xcb_motion_notify_event_t * e = (xcb_motion_notify_event_t *)gen_e;
	struct xeventst * xe = GC_MALLOC(sizeof(struct xeventst));
	xe->xe_type = e->response_type & ~0x80;
	xe->xe_window = e->event;
	xe->xe_x = e->event_x;
	xe->xe_y = e->event_y;
	xe->xe_state = e->state;
	xe->xe_button = e->detail;
	xe->xe_time = e->time;
	push_xevent(xe);
}

static int_fast16_t handle_xev(xcb_generic_event_t * restrict event,
	int_fast16_t * restrict count, const uint8_t flags)
{
	uint8_t * s;

	switch (event->response_type & ~0x80) {
	case XCB_KEY_PRESS:
		s = lookup_key(event, count);
		if (count)
			  send_string(s, *count);
		break;
	case XCB_FOCUS_IN:
	case XCB_FOCUS_OUT:
		handle_focus((xcb_focus_in_event_t *)event);
		break;
	case XCB_SELECTION_REQUEST:
		handle_sel_req((xcb_selection_request_event_t *)event);
		break;
	case XCB_SELECTION_NOTIFY:
		handle_sel_not((xcb_selection_notify_event_t *)event);
		break;
	case XCB_CLIENT_MESSAGE:
		handle_client_msg((xcb_client_message_event_t *)event);
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		handle_expose((xcb_expose_event_t *)event);
		break;
	default:
		handle_other(event);
	}
	if (flags & GET_XEVENTS)
		  return GCC_NULL;
	return 0;
}

static int_fast16_t output_to_command(int_fast16_t count)
{
	count = MIN(jbxvt.com.send_count, 100);
#ifdef SYS_write
	count = syscall(SYS_write, jbxvt.com.fd,
		jbxvt.com.send_nxt, count);
#else//!SYS_write
	count = write(jbxvt.com.fd,
		jbxvt.com.send_nxt,count);
#endif//SYS_write
	if (count < 0)
		  quit(1, WARN_RES RES_CMD);
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
		  FD_SET(jbxvt.com.fd,&out_fdset);
	int sv;
#ifdef SYS_select
	sv = syscall(SYS_select, jbxvt.com.width,
		in_fdset, &out_fdset, NULL, NULL);
#else//!SYS_select
	sv = select(jbxvt.com.width, in_fdset,&out_fdset, NULL, NULL);
#endif//SYS_select
	if (sv != -1 && FD_ISSET(jbxvt.com.fd,&out_fdset)) {
		count = output_to_command(count);
	}
	return count ;
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
			const int_fast16_t xev_ret = handle_xev(e, &count, flags);
			free(e);
			if (xev_ret)
				  return xev_ret;
		}
		count = poll_io(count, &in_fdset);
	} while(!FD_ISSET(jbxvt.com.fd,&in_fdset));
#ifdef SYS_read
	count = syscall(SYS_read, jbxvt.com.fd,
		jbxvt.com.buf.data, COM_BUF_SIZE);
#else//!SYS_read
	count = read(jbxvt.com.fd, jbxvt.com.buf.data, COM_BUF_SIZE);
#endif//SYS_read
	if (count < 1) // buffer is empty
		return errno == EWOULDBLOCK ? GCC_NULL : EOF;
	jbxvt.com.buf.next = jbxvt.com.buf.data;
	jbxvt.com.buf.top = jbxvt.com.buf.data + count;
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
	tk->tk_nlcount = 0;
	do {
		tk->tk_string[i++] = c;
		c = get_com_char(1);
		if (c == '\n' && ++tk->tk_nlcount >= NLMAX) {
			--tk->tk_nlcount;
			break;
		}
	} while (is_string_char(c) && i < TKS_MAX);
	tk->tk_length = i;
	tk->tk_string[i] = 0;
	tk->tk_type = TK_STRING;
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
		tk->tk_private = c;
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
			  tk->tk_arg[i++] = n;
		if (c == ESC)
			  push_com_char(c);
		if (c < ' ')
			  return;
		if (c < '@')
			  c = get_com_char(0);
	} while (c < '@' && c >= ' ');
	if (c == ESC)
		  push_com_char(c);
	tk->tk_nargs = i;
	tk->tk_type = c;
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
	tk->tk_arg[0] = n;
	tk->tk_nargs = 1;
	c = get_com_char(0);
	register uint_fast16_t i = 0;
	while ((c & 0177) >= ' ' && i < TKS_MAX) {
		if (c >= ' ')
			  tk->tk_string[i++] = c;
		c = get_com_char(0);
	}
	tk->tk_length = i;
	tk->tk_string[i] = 0;
	tk->tk_type = TK_TXTPAR;
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
	case '[':
		start_esc(c, tk);
		break;
	case ']':
		end_esc(c, tk);
		break;
	case '#':
	case '(':
	case ')':
		tk->tk_type = c;
		c = get_com_char(0);
		tk->tk_arg[0] = c;
		tk->tk_nargs = 1;
		break;
	case '7':
	case '8':
	case '=':
	case '>':
		tk->tk_type = c;
		tk->tk_nargs = 0;
		break;
	case 'D' :
		tk->tk_type = TK_IND;
		break;
	case 'E' :
		tk->tk_type = TK_NEL;
		break;
	case 'F': // Enter VT52 graphics mode
		tk->tk_type = TK_ENTGM52;
		break;
	case 'G': // Leave VT52 graphics mode
		tk->tk_type = TK_EXTGM52;
		break;
	case 'H' :
		tk->tk_type = TK_HTS;
		break;
	case 'M' :
		tk->tk_type = TK_RI;
		break;
	case 'N' :
		tk->tk_type = TK_SS2;
		break;
	case 'O' :
		tk->tk_type = TK_SS3;
		break;
	case 'Z' :
		tk->tk_type = TK_DECID;
		break;
	case 'c': // Reset to Initial State
		tk->tk_type = TK_RIS;
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
	if (c == GCC_NULL) {
		tk->tk_type = TK_NULL;
	} else if (c == EOF) {
		tk->tk_type = TK_EOF;
	} else if (is_string_char(c)) {
		handle_string_char(c, tk);
	} else if (c == ESC) {
		handle_esc(c, tk);
	} else {
		tk->tk_type = TK_CHAR;
		tk->tk_char = c;
	}
}

