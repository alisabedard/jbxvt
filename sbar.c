// Changes copyright 2016, Jeffrey E. Bedard <jefbed@gmail.com>
/*  Copyright 1992 John Bovey, University of Kent at Canterbury.
 *
 *  Redistribution and use in source code and/or executable forms, with
 *  or without modification, are permitted provided that the following
 *  condition is met:
 *
 *  Any redistribution must retain the above copyright notice, this
 *  condition and the following disclaimer, either as part of the
 *  program source code included in the redistribution or in human-
 *  readable materials provided with the redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS".  Any express or implied
 *  warranties concerning this software are disclaimed by the copyright
 *  holder to the fullest extent permitted by applicable law.  In no
 *  event shall the copyright-holder be liable for any damages of any
 *  kind, however caused and on any theory of liability, arising in any
 *  way out of the use of, or inability to use, this software.
 *
 *  -------------------------------------------------------------------
 *
 *  In other words, do not misrepresent my work as your own work, and
 *  do not sue me if it causes problems.  Feel free to do anything else
 *  you wish with it.
 */

#include "sbar.h"

#include "jbxvt.h"

static struct {
	uint16_t width, height;
	uint16_t mtop, mbot; // marked area
	// most recent arguments to sbar_show:
	uint8_t last_length, last_low, last_high;
} sbar = { .last_length = 100, .last_high = 100 };

//  Redraw the scrollbar after a size change
void sbar_reset(void)
{
	int32_t d;
	uint32_t u;

	XGetGeometry(jbxvt.X.dpy, jbxvt.X.win.sb, &(Window){0},&d,&d,
		(unsigned int *)&sbar.width,
		(unsigned int *)&sbar.height, &u, &u);
	sbar.mbot = -1;	/* force a redraw */
	sbar_show(sbar.last_length, sbar.last_low, sbar.last_high);
}

/*  Redraw the scrollbar to show the area from low to high,
    proportional to length.  */
void sbar_show(const uint8_t length, const uint8_t low,
	const uint8_t high)
{
	uint16_t top, bot;

	if (!length) return;

	sbar.last_length = length;
	sbar.last_low = low;
	sbar.last_high = high;

	top = sbar.height - 1 - sbar.height * high / length;
	bot = sbar.height - 1 - sbar.height * low / length;

	if (top == sbar.mtop && bot == sbar.mbot)
		return;
	if (top > 0)
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.sb,0,0,
			sbar.width,top - 1, false);
	if (bot >= top)
		XFillRectangle(jbxvt.X.dpy, jbxvt.X.win.sb,
			jbxvt.X.gc.sb, 0, top, sbar.width,
			bot - top + 1);
	if (bot < sbar.height - 1)
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.sb,0,bot + 1,
			sbar.width, sbar.height - bot - 1, false);
}

