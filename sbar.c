/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "sbar.h"
#include "cursor.h"
#include "jbxvt.h"
#include "repaint.h"
#include <string.h>
xcb_window_t jbxvt_get_scrollbar(xcb_connection_t * c)
{
	static xcb_window_t sb;
	if (sb)
		return sb;
	return sb = xcb_generate_id(c);
}
__attribute__((pure))
static int16_t get_sz(const int16_t margin)
{
	return jbxvt.scr.pixels.h - jbxvt.scr.pixels.h
		* (jbxvt.scr.offset + margin)
		/ (jbxvt.scr.sline.top + jbxvt.scr.chars.h);
}
// Draw the scrollbar.
void jbxvt_draw_scrollbar(xcb_connection_t * xc)
{
	xcb_clear_area(xc, 0, jbxvt_get_scrollbar(xc), 0, 0,
		JBXVT_SCROLLBAR_WIDTH, jbxvt.scr.pixels.h);
	const int16_t top = get_sz(jbxvt.scr.chars.h);
	xcb_poly_fill_rectangle(xc, jbxvt_get_scrollbar(xc),
		jbxvt.X.gc.tx, 1, &(xcb_rectangle_t){0, top,
		JBXVT_SCROLLBAR_WIDTH, get_sz(0) - top});
}
//  Change the value of the scrolled screen offset and repaint the screen
void jbxvt_set_scroll(xcb_connection_t * xc, int16_t n)
{
	JB_LIMIT(n, jbxvt.scr.sline.top, 0);
	if (n == jbxvt.scr.offset)
		return;
	jbxvt.scr.offset = n;
	jbxvt_repaint();
	jbxvt_draw_cursor();
	jbxvt_draw_scrollbar(xc);
}
// Scroll to the specified y position (in pixels)
void jbxvt_scroll_to(xcb_connection_t * xc, const int16_t y)
{
	jbxvt_set_scroll(xc, (jbxvt.scr.chars.h + jbxvt.scr.sline.top)
			* (jbxvt.scr.pixels.h - y) / jbxvt.scr.pixels.h
			- jbxvt.scr.chars.h);
}
void jbxvt_clear_saved_lines(xcb_connection_t * xc)
{
	memset(jbxvt.scr.sline.data, 0,
		sizeof(struct JBXVTSavedLine) * JBXVT_MAX_SCROLL);
	jbxvt.scr.sline.top = 0;
	jbxvt_set_scroll(xc, 0);
}
void jbxvt_toggle_scrollbar(xcb_connection_t * xc)
{
	xcb_configure_window(xc, jbxvt.X.win.vt,
		XCB_CONFIG_WINDOW_X, &(uint32_t){(
		jbxvt.opt.show_scrollbar^=true)
		? JBXVT_SCROLLBAR_WIDTH : 0});
}
