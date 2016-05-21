/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cursor.h"

#include "config.h"
#include "jbxvt.h"
#include "screen.h"

static uint8_t cursor_focus;	/* window has the keyboard focus */

//  Draw the cursor at the current position.
void cursor(void)
{
	if (jbxvt.scr.offset > 0)
		return;

	const int16_t x = MARGIN + jbxvt.X.font_width * jbxvt.scr.current->col;
	const int16_t y = MARGIN + jbxvt.X.font_height * jbxvt.scr.current->row;
	XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.cu,
		x,y,jbxvt.X.font_width,jbxvt.X.font_height);
	if (!cursor_focus)
		XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.cu,
			x + 1,y + 1, jbxvt.X.font_width - 2,
			jbxvt.X.font_height - 2);
}

/*  Indicate a change of keyboard focus.  Type is 1 if focusing in,
    2 for entry events, and 4 for focus events.  */
void scr_focus(const uint8_t flags)
{
	cursor(); // clear via invert gc
	if(flags & SCR_FOCUS_IN)
		  cursor_focus |= flags>>1;
	else
		  cursor_focus &= ~(flags>>1);
	cursor(); // draw
}

