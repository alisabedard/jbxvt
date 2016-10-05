/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "sbar.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "screen.h"
#include "xsetup.h"

#include <stdlib.h>
#include <string.h>

static struct {
	uint16_t height;
	// most recent arguments to sbar_show:
	int16_t last_low, last_high, last_length;
} sbar = {.last_length = 100, .last_high = 100 };

//  Redraw the scrollbar after a size change
void sbar_reset(void)
{
	sbar.height = jbxvt.scr.pixels.height;
	sbar_draw(sbar.last_length, sbar.last_low, sbar.last_high);
}

static int16_t get_sz(const int16_t lh, const uint16_t length)
{
	return sbar.height - sbar.height * lh / length;
}

/*  Redraw the scrollbar to show the area from low to high,
    proportional to length.  */
void sbar_draw(uint16_t length, const int16_t low,
	const int16_t high)
{
	if (!jbxvt.opt.show_scrollbar || !length)
		  return;
	sbar.last_length = length;
	sbar.last_low = low;
	sbar.last_high = high;
	const int16_t top = get_sz(high, length), bot = get_sz(low, length);
	const xcb_window_t sb = jbxvt.X.win.sb;
	if (top > 0)
		xcb_clear_area(jbxvt.X.xcb, false, sb, 0, 0,
			SBAR_WIDTH, top - 1);
	if (bot >= top)
		xcb_poly_fill_rectangle(jbxvt.X.xcb, sb, jbxvt.X.gc.tx, 1,
			&(xcb_rectangle_t){0, top, SBAR_WIDTH,
			bot - top + 1});
	if (bot < sbar.height)
		xcb_clear_area(jbxvt.X.xcb, false, sb, 0, bot + 1,
			SBAR_WIDTH, sbar.height - bot - 1);
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
	sbar_draw(CSZ.h + t - 1, n, n + CSZ.h - 1);
}

void jbxvt_clear_saved_lines(void)
{
	change_offset(0);
	memset(jbxvt.scr.sline.data, 0,
		sizeof(struct JBXVTSavedLine) * JBXVT_MAX_SCROLL);
	jbxvt.scr.sline.top = 0;
}

static void set_vt_x(const int8_t x)
{
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt,
		XCB_CONFIG_WINDOW_X, &(uint32_t){x});
	(x ? xcb_map_window : xcb_unmap_window)(jbxvt.X.xcb, jbxvt.X.win.sb);
}

void jbxvt_show_sbar(void)
{
	set_vt_x(SBAR_WIDTH);
}

void jbxvt_hide_sbar(void)
{
	set_vt_x(0);
}

void jbxvt_toggle_sbar(void)
{
	(jbxvt.opt.show_scrollbar ^= true)
		? jbxvt_show_sbar() : jbxvt_hide_sbar();
}

