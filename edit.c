/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
//#undef DEBUG
#include "edit.h"
#include <string.h>
#include "cursor.h"
#include "font.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "paint.h"
#include "repaint.h"
#include "scr_reset.h"
#include "screen.h"
#include "selection.h"
#include "size.h"
#include "window.h"
static uint16_t get_copy_width(const uint8_t count,
	const uint8_t font_width)
{
	const uint16_t cw = jbxvt_get_char_size().w - 1;
	const int16_t x = jbxvt_get_x();
	const uint16_t w = cw - count - x - 1;
	return w * font_width;
}
static uint16_t copy_area(xcb_connection_t * restrict xc,
	const int16_t * restrict x, const uint8_t count)
{
	const xcb_window_t v = jbxvt_get_vt_window(xc);
	const struct JBDim f = jbxvt_get_font_size();
	const int16_t y = jbxvt_get_y() * f.height;
	const uint16_t w = get_copy_width(count, f.w);
	const xcb_gc_t gc = jbxvt_get_text_gc(xc);
	const uint16_t px = f.w * x[1];
	xcb_copy_area(xc, v, v, gc, f.w * x[0], y, px, y, w, f.h);
	return w;
}
void jbxvt_edit_characters(xcb_connection_t * xc,
	const uint8_t count, const bool delete)
{
	const int16_t x = jbxvt_get_x();
	LOG("jbxvt_edit_characters(count: %d, delete: %s, x: %d)",
		count, delete ? "true" : "false", x);
	const int16_t y = jbxvt_get_y();
	jbxvt_check_selection(xc, y, y);
	struct JBXVTLine * l = jbxvt_get_line(y);
	uint8_t * t = l->text;
	uint8_t * a = t + x;
	if (x + count >= JBXVT_MAX_COLUMNS) {
		LOG("\tWARNING: attempted to write outside of buffer");
		return; // Stay within bounds!
	}
	uint8_t * b = a + count;
	if (delete) {
		JB_SWAP(uint8_t *, a, b);
		l->wrap = false;
	}
	memmove(a, b, count);
	const int16_t p[] = {a - t, b - t};
	LOG("\tp[0]: %d, p[1]: %d", p[0], p[1]);
	if (delete) {
		const struct JBDim f = jbxvt_get_font_size();
		const xcb_window_t v = jbxvt_get_vt_window(xc);
		xcb_clear_area(xc, 0, v, x*f.w, y*f.h,
			count*f.w, f.h);
	}
	copy_area(xc, p, count);
	jbxvt_draw_cursor(xc);
}
