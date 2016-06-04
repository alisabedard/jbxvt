/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "sbar.h"

#include "jbxvt.h"
#include "log.h"

#include <stdlib.h>

static struct {
	Size sz;
	int mtop, mbot; // marked area
	// most recent arguments to sbar_show:
	int last_low, last_high, last_length;
} sbar = { .last_length = 100, .last_high = 100 };

//  Redraw the scrollbar after a size change
void sbar_reset(void)
{
#ifdef USE_XCB
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(jbxvt.X.xcb,
		xcb_get_geometry(jbxvt.X.xcb, jbxvt.X.win.sb), NULL);
	sbar.sz.w = r->width;
	sbar.sz.h = r->height;
	free(r);
#else//!USE_XCB
	int d;
	unsigned int w, h, u;

	XGetGeometry(jbxvt.X.dpy, jbxvt.X.win.sb, &(Window){0},
		&d, &d, &w, &h, &u, &u);
	sbar.sz.w = w;
	sbar.sz.h = h;
#endif//USE_XCB
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

	int top = sbar.sz.height - sbar.sz.height*high/length;
	int bot = sbar.sz.height - sbar.sz.height*low/length;

	if (top == sbar.mtop && bot == sbar.mbot)
		return;
	if (top > 0) {
#ifdef USE_XCB
		xcb_clear_area(jbxvt.X.xcb, false, jbxvt.X.win.sb, 0, 0,
			sbar.sz.width, top - 1);
#else//!USE_XCB
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.sb,0,0,
			sbar.sz.width,top - 1, false);
#endif//USE_XCB
	}

	if (bot >= top) {
#ifdef USE_XCB
		xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.sb,
			jbxvt.X.gc.sb, 1, &(xcb_rectangle_t){0, top,
			sbar.sz.width, bot - top + 1});
#else//!USE_XCB
		XFillRectangle(jbxvt.X.dpy, jbxvt.X.win.sb,
			jbxvt.X.gc.sb, 0, top, sbar.sz.width,
			bot - top + 1);
#endif//USE_XCB
	}

	if (bot < sbar.sz.height) {
#ifdef USE_XCB
		xcb_clear_area(jbxvt.X.xcb, false, jbxvt.X.win.sb,
			0, bot + 1, sbar.sz.width, sbar.sz.height - bot - 1);
#else//!USE_XCB
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.sb,0,bot + 1,
			sbar.sz.width, sbar.sz.height - bot - 1, false);
#endif//USE_XCB
	}
	LOG("L:%d, l:%d, h:%d, t:%d, b:%d, h:%d\n", length, low, high,
		top, bot, sbar.sz.height);
}

