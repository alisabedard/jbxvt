#include "cmdtok.h"

#include "command.h"

#include "jbxvt.h"
#include "screen.h"
#include "token.h"
#include "ttyinit.h"
#include "wm_del_win.h"
#include "xevents.h"
#include "xeventst.h"
#include "xsetup.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>


static int x_fd;

void init_cmdtok(void)
{
	  x_fd = XConnectionNumber(jbxvt.X.dpy);
}

/*  Return the next input character after first passing any keyboard input
 *  to the command.  If flags & BUF_ONLY is true then only buffered characters are
 *  returned and once the buffer is empty the special value GCC_NULL is
 *  returned.  If flags and GET_XEVENTS is true then GCC_NULL is returned
 *  when an X event arrives.
 */
static int16_t get_com_char(const int8_t flags)
{
	XEvent event;
	struct xeventst *xe;
	fd_set in_fdset, out_fdset;
	unsigned char *s;
	int count, sv;
	unsigned char mask = is_eightbit() ? 0xff : 0x7f;
	extern int errno;

	if (jbxvt.com.stack.top > jbxvt.com.stack.data)
		return(*--jbxvt.com.stack.top);

	if (jbxvt.com.buf.next < jbxvt.com.buf.top)
		return(*jbxvt.com.buf.next++ & mask);
	else if (flags & BUF_ONLY)
		return(GCC_NULL);

	for (;;) {
		FD_ZERO(&in_fdset);
		while (XPending(jbxvt.X.dpy) == 0) {
			if (FD_ISSET(x_fd,&in_fdset))
				/*  If we get to this point something is wrong
				 *  because there is X input available but no
				 *  events.  Exit the program to avoid looping
				 *  forever.
				 */
				quit(0);
			FD_SET(jbxvt.com.fd,&in_fdset);
			FD_SET(x_fd,&in_fdset);
			FD_ZERO(&out_fdset);
			if (jbxvt.com.send_count > 0)
				FD_SET(jbxvt.com.fd,&out_fdset);
			do
				sv = select(jbxvt.com.width,
					&in_fdset,&out_fdset,
					NULL,NULL);
			while (sv < 0 && errno == EINTR);
			if (sv < 0) {
				error("select failed");
				quit(-1);
			}

			if (FD_ISSET(jbxvt.com.fd,&out_fdset)) {
				count = jbxvt.com.send_count < 100
					? jbxvt.com.send_count : 100;
				count = write(jbxvt.com.fd,jbxvt.com.send_nxt,count);
				if (count < 0) {
					error("failed to write to command");
					quit(-1);
				}
				jbxvt.com.send_count -= count;
				jbxvt.com.send_nxt += count;
			}

			if (FD_ISSET(jbxvt.com.fd,&in_fdset))
				break;
		}
		if (FD_ISSET(jbxvt.com.fd,&in_fdset))
			break;
		XNextEvent(jbxvt.X.dpy,&event);
		if (event.type == KeyPress) {
			s = lookup_key(&event,&count);
			if (count != 0)
				send_string(s,count);
		} else if (event.type == ClientMessage) {
			if (event.xclient.format == 32
				&& event.xclient.data.l[0]
				== (long)get_wm_del_win())
				quit(0);
		} else if (event.type == MappingNotify) {
			XRefreshKeyboardMapping(&event.xmapping);
		} else if (event.type == SelectionRequest) {
			xe = (struct xeventst *)malloc(sizeof(struct xeventst));
			xe->xe_type = event.type;
			xe->xe_window = event.xselectionrequest.owner;
			xe->xe_time = event.xselectionrequest.time;
			xe->xe_requestor = event.xselectionrequest.requestor;
			xe->xe_target = event.xselectionrequest.target;
			xe->xe_property = event.xselectionrequest.property;
			push_xevent(xe);
			if (flags & GET_XEVENTS)
				return(GCC_NULL);
		} else if (event.type == SelectionNotify) {
			xe = (struct xeventst *)malloc(sizeof(struct xeventst));
			xe->xe_type = event.type;
			xe->xe_time = event.xselection.time;
			xe->xe_requestor = event.xselection.requestor;
			xe->xe_property = event.xselection.property;
			push_xevent(xe);
			if (flags & GET_XEVENTS)
				return(GCC_NULL);
		} else if (event.type == FocusIn || event.type == FocusOut) {
			if (event.xfocus.mode != NotifyNormal)
				continue;
			switch (event.xfocus.detail) {
			    case NotifyAncestor :
			    case NotifyInferior :
			    case NotifyNonlinear :
				break;
			    default :
				continue;
			}
			xe = (struct xeventst *)malloc(sizeof(struct xeventst));
			xe->xe_type = event.type;
			xe->xe_time = event.xselection.time;
			xe->xe_detail = event.xfocus.detail;
			push_xevent(xe);
			if (flags & GET_XEVENTS)
				return(GCC_NULL);
		} else {
			xe = (struct xeventst *)malloc(sizeof(struct xeventst));
			xe->xe_type = event.type;
			xe->xe_window = event.xany.window;
			if (event.type == Expose || event.type == GraphicsExpose) {
				xe->xe_x = event.xexpose.x;
				xe->xe_y = event.xexpose.y;
				xe->xe_width = event.xexpose.width;
				xe->xe_height = event.xexpose.height;
			} else {
				xe->xe_time = event.xbutton.time;
				xe->xe_x = event.xbutton.x;
				xe->xe_y = event.xbutton.y;
				xe->xe_state = event.xbutton.state;
				xe->xe_button = event.xbutton.button;
			}
			push_xevent(xe);
			if (flags & GET_XEVENTS)
				return(GCC_NULL);
		}
	}

	count = read(jbxvt.com.fd,jbxvt.com.buf.data,COM_BUF_SIZE);
	if (count <= 0)
		return errno == EWOULDBLOCK ? GCC_NULL : EOF;
	jbxvt.com.buf.next = jbxvt.com.buf.data;
	jbxvt.com.buf.top = jbxvt.com.buf.data + count;
	return *jbxvt.com.buf.next++ & mask;
}


//  Return an input token
void get_token(struct tokenst * restrict tk)
{
	int16_t c, i, n;

	tk->tk_private = 0;
	tk->tk_type = TK_NULL;
	if(handle_xevents(tk))
		  return;
	if ((c = get_com_char(GET_XEVENTS)) == GCC_NULL) {
		tk->tk_type = TK_NULL;
		return;
	}

	if (c == EOF) {
		tk->tk_type = TK_EOF;
		return;
	}
	if (is_string_char(c)) {
		i = 0;
		tk->tk_nlcount = 0;
		do {
			tk->tk_string[i++] = c;
			c = get_com_char(1);
			if (c == '\n' && ++tk->tk_nlcount >= NLMAX) {
				tk->tk_nlcount--;
				break;
			}
		} while (is_string_char(c) && i < TKS_MAX);
		tk->tk_length = i;
		tk->tk_string[i] = 0;
		tk->tk_type = TK_STRING;
		if (c != GCC_NULL)
			push_com_char(c);
	} else if (c == ESC) {
		c = get_com_char(0);
		if (c == '[') {
			c = get_com_char(0);
			if (c >= '<' && c <= '?') {
				tk->tk_private = c;
				c = get_com_char(0);
			}

			/*  read any numerical arguments
			 */
			i = 0;
			do {
				n = 0;
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
			if (c < ' ')
				return;
			tk->tk_nargs = i;
			tk->tk_type = c;
		} else if (c == ']') {
			c = get_com_char(0);
			n = 0;
			while (c >= '0' && c <= '9') {
				n = n * 10 + c - '0';
				c = get_com_char(0);
			}
			tk->tk_arg[0] = n;
			tk->tk_nargs = 1;
			c = get_com_char(0);
			i = 0;
			while ((c & 0177) >= ' ' && i < TKS_MAX) {
				if (c >= ' ')
					tk->tk_string[i++] = c;
				c = get_com_char(0);
			}
			tk->tk_length = i;
			tk->tk_string[i] = 0;
			tk->tk_type = TK_TXTPAR;
		} else if (c == '#' || c == '(' || c == ')') {
			tk->tk_type = c;
			c = get_com_char(0);
			tk->tk_arg[0] = c;
			tk->tk_nargs = 1;
		} else if (c == '7' || c == '8' || c == '=' || c == '>') {
			tk->tk_type = c;
			tk->tk_nargs = 0;
		} else {
			switch (c) {
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
	} else {
		tk->tk_type = TK_CHAR;
		tk->tk_char = c;
	}
}

