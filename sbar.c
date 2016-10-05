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

#undef SB
#define SB jbxvt.X.win.sb

static int16_t get_sz(const int16_t lh, const uint16_t length)
{
	return PSZ.h - PSZ.h * lh / length;
}

// Draw the scrollbar.
void jbxvt_draw_scrollbar(void)
{
	uint16_t length = jbxvt.scr.sline.top + CSZ.h;
	int16_t low = jbxvt.scr.offset;
	int16_t high = low + CSZ.h;
	if (!jbxvt.opt.show_scrollbar || !length)
		  return;
	const int16_t top = get_sz(high, length), bot = get_sz(low, length);
	xcb_clear_area(jbxvt.X.xcb, 0, SB, 0, 0, SBAR_WIDTH, PSZ.h);
	xcb_poly_fill_rectangle(jbxvt.X.xcb, SB, jbxvt.X.gc.tx, 1,
			&(xcb_rectangle_t){0, top, SBAR_WIDTH,
			bot - top});
}

//  Change the value of the scrolled screen offset and repaint the screen
void jbxvt_set_scroll(int16_t n)
{
	const int32_t t = jbxvt.scr.sline.top;
	n = MIN(MAX(n, 0), t);
	if (n == jbxvt.scr.offset)
		return;
	jbxvt.scr.offset = n;
	draw_cursor(); // clear
	repaint();
	draw_cursor(); // draw
	jbxvt_draw_scrollbar();
}

void jbxvt_clear_saved_lines(void)
{
	jbxvt_set_scroll(0);
	memset(jbxvt.scr.sline.data, 0,
		sizeof(struct JBXVTSavedLine) * JBXVT_MAX_SCROLL);
	jbxvt.scr.sline.top = 0;
}

static void set_vt_x(const int8_t x)
{
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt,
		XCB_CONFIG_WINDOW_X, &(uint32_t){x});
}

void jbxvt_show_sbar(void)
{
	set_vt_x(SBAR_WIDTH);
	xcb_map_window(jbxvt.X.xcb, SB);
}

void jbxvt_hide_sbar(void)
{
	set_vt_x(0);
	xcb_unmap_window(jbxvt.X.xcb, SB);
}

void jbxvt_toggle_sbar(void)
{
	(jbxvt.opt.show_scrollbar ^= true)
		? jbxvt_show_sbar() : jbxvt_hide_sbar();
}

