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
#include "size.h"
#include "window.h"
static uint16_t get_copy_width(const uint8_t count, const uint8_t
	font_width)
{
	const uint16_t w = jbxvt_get_char_size().w - count - jbxvt_get_x();
	return w * font_width;
}
static void copy_area(xcb_connection_t * restrict xc,
	const int16_t * restrict x, const uint8_t count)
{
	const xcb_window_t v = jbxvt_get_vt_window(xc);
	const struct JBDim f = jbxvt_get_font_size();
	const int16_t y = jbxvt_get_y() * f.height;
	xcb_copy_area(xc, v, v, jbxvt_get_text_gc(xc), f.w * x[0], y,
		f.w * x[1], y, get_copy_width(count, f.w), f.h);
}
void jbxvt_edit_characters(xcb_connection_t * xc,
	const uint8_t count, const bool delete)
{
	LOG("jbxvt_edit_characters(count: %d, delete: %s)", count,
		delete ? "true" : "false");
	struct JBXVTLine * l = jbxvt_get_line(jbxvt_get_y());
	const int16_t x = jbxvt_get_x();
	uint8_t * t = l->text;
	uint8_t * a = t + x;
	uint8_t * b = a + count;
	if (delete)
		JB_SWAP(uint8_t *, a, b);
	memmove(a, b, count);
	const int16_t p[] = {a - t, b - t};
	copy_area(xc, p, count);
	jbxvt_draw_cursor(xc);
}
