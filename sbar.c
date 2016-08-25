/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "sbar.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"

#include <stdlib.h>

static struct {
	struct JBSize16 sz;
	// most recent arguments to sbar_show:
	int16_t last_low, last_high, last_length;
} sbar = { .sz.width = SBAR_WIDTH, .last_length = 100, .last_high = 100 };

//  Redraw the scrollbar after a size change
void sbar_reset(void)
{
	sbar.sz.height = jbxvt.scr.pixels.height;
	sbar_show(sbar.last_length, sbar.last_low, sbar.last_high);
}

static int16_t get_sz(const int16_t lh, const uint16_t length)
{
	return sbar.sz.h - sbar.sz.h * lh / length;
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
	const struct JBSize16 s = sbar.sz;
	const int16_t top = get_sz(high, length), bot = get_sz(low, length);
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

//  Change the value of the scrolled screen offset and repaint the screen
void change_offset(int16_t n)
{
	const int32_t t = jbxvt.scr.sline.top;
	n = MIN(MAX(n, 0), t);
	if (n == jbxvt.scr.offset)
		return;
	jbxvt.scr.offset = n;
	draw_cursor(); // clear
	repaint();
	draw_cursor(); // draw
#define C jbxvt.scr.chars
	sbar_show(C.h + t - 1, n, n + C.h - 1);
}

