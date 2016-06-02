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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>


static int16_t handle_xev(XEvent * event, int16_t * restrict count,
	const int8_t flags)
{
	struct xeventst * xe;
	uint8_t * s;

	switch (event->type) {
	case KeyPress:
		s = lookup_key(event, count);
		if (count) send_string(s, *count);
		break;
	case FocusIn:
	case FocusOut:
		if (event->xfocus.mode != NotifyNormal)
			  return 0;
		switch (event->xfocus.detail) {
		case NotifyAncestor :
		case NotifyInferior :
		case NotifyNonlinear :
			break;
		default :
			return 0;
		}
		xe = calloc(1, sizeof(struct xeventst));
		xe->xe_type = event->type;
		xe->xe_time = event->xselection.time;
		xe->xe_detail = event->xfocus.detail;
		push_xevent(xe);
		if (flags & GET_XEVENTS)
			  return(GCC_NULL);
		break;
	case SelectionRequest:
		xe = (struct xeventst *)malloc(sizeof(struct xeventst));
		xe->xe_type = event->type;
		xe->xe_window = event->xselectionrequest.owner;
		xe->xe_time = event->xselectionrequest.time;
		xe->xe_requestor = event->xselectionrequest.requestor;
		xe->xe_target = event->xselectionrequest.target;
		xe->xe_property = event->xselectionrequest.property;
		push_xevent(xe);
		if (flags & GET_XEVENTS)
			  return(GCC_NULL);
		break;
	case SelectionNotify:
		xe = (struct xeventst *)malloc(sizeof(struct xeventst));
		xe->xe_type = event->type;
		xe->xe_time = event->xselection.time;
		xe->xe_requestor = event->xselection.requestor;
		xe->xe_property = event->xselection.property;
		push_xevent(xe);
		if (flags & GET_XEVENTS)
			  return(GCC_NULL);
		break;

	case ClientMessage:
		if (event->xclient.format == 32
			&& event->xclient.data.l[0]
			== (long)wm_del_win())
			  quit(0, NULL);
		break;
	case MappingNotify:
		XRefreshKeyboardMapping(&event->xmapping);
		break;
	default:
		xe = (struct xeventst *)malloc(sizeof(struct xeventst));
		xe->xe_type = event->type;
		xe->xe_window = event->xany.window;
		if (event->type == Expose
			|| event->type == GraphicsExpose) {
			xe->xe_x = event->xexpose.x;
			xe->xe_y = event->xexpose.y;
			xe->xe_width = event->xexpose.width;
			xe->xe_height = event->xexpose.height;
		} else {
			xe->xe_time = event->xbutton.time;
			xe->xe_x = event->xbutton.x;
			xe->xe_y = event->xbutton.y;
			xe->xe_state = event->xbutton.state;
			xe->xe_button = event->xbutton.button;
		}
		push_xevent(xe);
		if (flags & GET_XEVENTS)
			  return GCC_NULL;
	}
	return 0;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((regparm(1)))
#endif//x86
static int16_t x_io_loop(int16_t count, fd_set * restrict in_fdset)
{
#ifdef USE_XCB
	const fd_t x_fd = xcb_get_file_descriptor(jbxvt.X.xcb);
#else//!USE_XCB
	const fd_t x_fd = XConnectionNumber(jbxvt.X.dpy);
#endif//USE_XCB
	while (XPending(jbxvt.X.dpy) == 0) {
		FD_SET(jbxvt.com.fd, in_fdset);
		FD_SET(x_fd, in_fdset);
		fd_set out_fdset;
		FD_ZERO(&out_fdset);
		if (jbxvt.com.send_count > 0)
			  FD_SET(jbxvt.com.fd,&out_fdset);
		int sv;
		do {
#ifdef SYS_select
			sv = syscall(SYS_select, jbxvt.com.width,
				in_fdset, &out_fdset, NULL, NULL);
#else//!SYS_select
			sv = select(jbxvt.com.width,
				in_fdset,&out_fdset,
				NULL, NULL);
#endif//SYS_select
		} while (sv < 0 && errno == EINTR);

		if (FD_ISSET(jbxvt.com.fd,&out_fdset)) {
			count = jbxvt.com.send_count < 100
				? jbxvt.com.send_count : 100;
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
		}
		if (FD_ISSET(jbxvt.com.fd, in_fdset))
			  break;
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
static int16_t get_com_char(const int8_t flags)
{
	if (jbxvt.com.stack.top > jbxvt.com.stack.data)
		return(*--jbxvt.com.stack.top);

	if (jbxvt.com.buf.next < jbxvt.com.buf.top)
		return(*jbxvt.com.buf.next++);

	if (flags & BUF_ONLY)
		return(GCC_NULL);

	int16_t count;
	fd_set in_fdset;
	XEvent event;
	register int16_t xev_ret;
wait_event_start:
	FD_ZERO(&in_fdset);
	count = x_io_loop(count, &in_fdset);
	if (FD_ISSET(jbxvt.com.fd,&in_fdset))
		  goto wait_event_end;
	XNextEvent(jbxvt.X.dpy,&event);
	if((xev_ret = handle_xev(&event, &count, flags)))
		  return xev_ret;
	goto wait_event_start;
wait_event_end:
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
	return(c >= ' ' || c == '\n' || c == '\r' || c == '\t');
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void handle_string_char(int_fast16_t c, struct tokenst * restrict tk)
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
static void start_esc(int_fast16_t c, struct tokenst * restrict tk)
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
			n *= 10;
			n += c - '0';
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
	if (c < ' ')
		  return;
	tk->tk_nargs = i;
	tk->tk_type = c;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void end_esc(int16_t c, struct tokenst * restrict tk)
{
	c = get_com_char(0);
	uint16_t n = 0;
	while (c >= '0' && c <= '9') {
		n = n * 10 + c - '0';
		c = get_com_char(0);
	}
	tk->tk_arg[0] = n;
	tk->tk_nargs = 1;
	c = get_com_char(0);
	register uint16_t i = 0;
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
static void handle_esc(int_fast16_t c, struct tokenst * restrict tk)
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
	default :
		return;
	}
}

//  Return an input token
void get_token(struct tokenst * restrict tk)
{
	memset(tk, 0, sizeof(struct tokenst));

	if(handle_xevents(tk))
		  return;

	int_fast16_t c = get_com_char(GET_XEVENTS);
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

