/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "sbar.h"

#include "jbxvt.h"
#include "log.h"

static struct {
	Dim d;
	int mtop, mbot; // marked area
	// most recent arguments to sbar_show:
	int last_low, last_high, last_length;
} sbar = { .last_length = 100, .last_high = 100 };

//  Redraw the scrollbar after a size change
void sbar_reset(void)
{
	int32_t d;
	uint32_t u;
	unsigned int w, h;

	XGetGeometry(jbxvt.X.dpy, jbxvt.X.win.sb, &(Window){0},
		&d, &d, &w, &h, &u, &u);
	sbar.d.w = w;
	sbar.d.h = h;
	sbar.mbot = -1;	/* force a redraw */
	sbar_show(sbar.last_length, sbar.last_low, sbar.last_high);
}

/*  Redraw the scrollbar to show the area from low to high,
    proportional to length.  */
void sbar_show(int length, const int low,
	const int high)
{
	if (!length) return;

	sbar.last_length = length;
	sbar.last_low = low;
	sbar.last_high = high;

	int top = sbar.d.height - sbar.d.height*high/length;
	int bot = sbar.d.height - sbar.d.height*low/length;

	if (top == sbar.mtop && bot == sbar.mbot)
		return;
	if (top > 0)
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.sb,0,0,
			sbar.d.width,top - 1, false);
	if (bot >= top)
		XFillRectangle(jbxvt.X.dpy, jbxvt.X.win.sb,
			jbxvt.X.gc.sb, 0, top, sbar.d.width,
			bot - top + 1);
	if (bot < sbar.d.height)
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.sb,0,bot + 1,
			sbar.d.width, sbar.d.height - bot - 1, false);
	LOG("L:%d, l:%d, h:%d, t:%d, b:%d, h:%d\n", length, low, high,
		top, bot, sbar.d.height);
}

