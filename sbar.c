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
	return PSZ.h - PSZ.h * (jbxvt.scr.offset + margin)
		/ (jbxvt.scr.sline.top + CSZ.h);
}

// Draw the scrollbar.
void jbxvt_draw_scrollbar(void)
{
	xcb_clear_area(jbxvt.X.xcb, 0, SB, 0, 0, SBAR_WIDTH, PSZ.h);
	const int16_t top = get_sz(CSZ.h);
	xcb_poly_fill_rectangle(jbxvt.X.xcb, SB, jbxvt.X.gc.tx, 1,
			&(xcb_rectangle_t){0, top, SBAR_WIDTH,
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

