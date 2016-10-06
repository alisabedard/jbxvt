/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "sbar.h"

#include "cursor.h"
#include "jbxvt.h"
#include "repaint.h"

#include <string.h>

#undef SB
#define SB jbxvt.X.win.sb

__attribute__((pure))
static int16_t get_sz(const int16_t margin)
{
	return jbxvt.scr.pixels.h - jbxvt.scr.pixels.h * (jbxvt.scr.offset + margin)
		/ (jbxvt.scr.sline.top + jbxvt.scr.chars.h);
}

// Draw the scrollbar.
void jbxvt_draw_scrollbar(void)
{
	xcb_clear_area(jbxvt.X.xcb, 0, SB, 0, 0, JBXVT_SCROLLBAR_WIDTH, jbxvt.scr.pixels.h);
	const int16_t top = get_sz(jbxvt.scr.chars.h);
	xcb_poly_fill_rectangle(jbxvt.X.xcb, SB, jbxvt.X.gc.tx, 1,
			&(xcb_rectangle_t){0, top, JBXVT_SCROLLBAR_WIDTH,
			get_sz(0) - top});
}

//  Change the value of the scrolled screen offset and repaint the screen
void jbxvt_set_scroll(int16_t n)
{
	JB_LIMIT(n, jbxvt.scr.sline.top, 0);
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

void jbxvt_toggle_scrollbar(void)
{
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt, XCB_CONFIG_WINDOW_X,
		&(uint32_t){(jbxvt.opt.show_scrollbar^=true)? JBXVT_SCROLLBAR_WIDTH : 0});
}

