/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "change_offset.h"

#include "cursor.h"
#include "jbxvt.h"
#include "repaint.h"
#include "sbar.h"

//  Change the value of the scrolled screen offset and repaint the screen
void change_offset(int16_t n)
{
	const int32_t t = jbxvt.scr.sline.top;
	n = MIN(MAX(n, 0), t);
	if (n == jbxvt.scr.offset)
		return;
	jbxvt.scr.offset = n;
	const Size c = jbxvt.scr.chars;
	cursor(CURSOR_DRAW); // clear
	repaint();
	cursor(CURSOR_DRAW); // draw
	sbar_show(c.h + t - 1, n, n + c.h - 1);
}

