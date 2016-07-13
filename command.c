/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.  */

#include "command.h"

#include "jbxvt.h"
#include "ttyinit.h"
#include "wm_del_win.h"
#include "xeventst.h"

#include <stdarg.h>
#include <stdio.h>

static struct {
	// start and end of queue:
	JBXVTEvent *start, *last;
} command;

//  Push a mini X event onto the queue
void push_xevent(JBXVTEvent * xe)
{
	xe->next = command.start;
	xe->prev = NULL;
	*(xe->next ? &xe->next->prev : &command.last) = xe;
}

JBXVTEvent * pop_xevent(void)
{
	JBXVTEvent * xe = command.last;
	if(xe) {
		command.last = xe->prev;
		*(command.last ? &command.last->next
			: &command.start) = NULL;
	}
	return xe;
}

/*  Initialize the command connection.  This should
    be called after the X server connection is established.  */
void init_command(char ** restrict argv)
{
	//  Enable the delete window protocol:
	wm_del_win();

	if ((jbxvt.com.fd = run_command(argv)) < 0)
		  quit(1, WARN_RES RES_SSN);
	jbxvt.com.buf.next = jbxvt.com.buf.top
		= jbxvt.com.buf.data;
	jbxvt.com.stack.top = jbxvt.com.stack.data;
}

//  Push an input character back into the input queue.
void push_com_char(const uint8_t c)
{
	if (jbxvt.com.stack.top < jbxvt.com.stack.data
		+ COM_PUSH_MAX)
		*jbxvt.com.stack.top++ = c;
}

/*  Send printf formatted output to the command.
    Only used for small ammounts of data.  */
void cprintf(char *fmt,...)
{
	va_list args;
	va_start(args,fmt);
	static uint8_t buf[7];
	// + 1 to include \0 terminator.
	const int l = vsnprintf((char *)buf, sizeof(buf),
		fmt, args) + 1;
	va_end(args);
	jbxvt.com.send_nxt = buf;
	jbxvt.com.send_count = l;
}

