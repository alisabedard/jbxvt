/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "sbar.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"

#include <stdlib.h>

static struct {
	Size sz;
	// most recent arguments to sbar_show:
	int16_t last_low, last_high, last_length;
} sbar = { .sz.width = SBAR_WIDTH, .last_length = 100, .last_high = 100 };

//  Redraw the scrollbar after a size change
void sbar_reset(void)
{
	sbar.sz.height = jbxvt.scr.pixels.height;
	sbar_show(sbar.last_length, sbar.last_low, sbar.last_high);
}

/*  Redraw the scrollbar to show the area from low to high,
    proportional to length.  */
void sbar_show(uint16_t length, const int16_t low,
	const int16_t high)
{
	if (!jbxvt.opt.show_scrollbar || !length)
		  return;
	sbar.last_length = length;
	sbar.last_low = low;
	sbar.last_high = high;
	const Size s = sbar.sz;
	const int16_t top = s.h - s.h * high / length;
	const int16_t bot = s.h - s.h * low / length;
	const xcb_window_t sb = jbxvt.X.win.sb;
	if (top > 0)
		xcb_clear_area(jbxvt.X.xcb, false, sb, 0, 0, s.w, top - 1);
	if (bot >= top)
		xcb_poly_fill_rectangle(jbxvt.X.xcb, sb, jbxvt.X.gc.tx, 1,
			&(xcb_rectangle_t){0, top, s.w, bot - top + 1});
	if (bot < sbar.sz.height)
		xcb_clear_area(jbxvt.X.xcb, false, sb, 0, bot + 1,
			s.w, s.h - bot - 1);
}

