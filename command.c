/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.  */

#include "command.h"

#include "jbxvt.h"
#include "ttyinit.h"
#include "xevents.h"
#include "xeventst.h"

#include <gc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
	jbxvt.com.fd = run_command(argv);
	if (jb_check(jbxvt.com.fd >= 0, "Could not start session"))
		exit(1);
	struct JBXVTCommandContainer * b = &jbxvt.com.buf,
				     * s = &jbxvt.com.stack;
	b->data = b->next = b->top = GC_MALLOC(COM_BUF_SIZE);
	s->data = s->top = GC_MALLOC(COM_PUSH_MAX);
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
	static uint8_t buf[32];
	// + 1 to include \0 terminator.
	const int l = vsnprintf((char *)buf, sizeof(buf),
		fmt, args) + 1;
	va_end(args);
	jbxvt.com.send_nxt = buf;
	jbxvt.com.send_count = l;
}

