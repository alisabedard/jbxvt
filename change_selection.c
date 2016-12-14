/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "change_selection.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <xcb/xproto.h>
#include "cursor.h"
#include "font.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "selend.h"
#include "selection.h"
#include "size.h"
#include "window.h"
static void invert(xcb_connection_t * xc, const int16_t rs,
	const int16_t re, const int16_t cs, const int16_t ce,
	const uint8_t row1, const uint8_t row2)
{
	const struct JBDim csz = jbxvt_get_char_size(),
	      fsz = jbxvt_get_font_size();
	for (uint8_t row = row1; row <= row2; row++) {
		const int16_t y = row * fsz.height;
		const int16_t x1 = row == rs ? cs * fsz.w : 0;
		const int16_t x2 = (row == re ? ce : csz.w) * fsz.w;
		xcb_poly_fill_rectangle(xc, jbxvt_get_vt_window(xc),
			jbxvt_get_cursor_gc(xc), 1, &(xcb_rectangle_t){
			.x = x1, .y = y, .width = x2 - x1, .height = fsz.h});
	}
}
static uint8_t get_row1(const int16_t rs)
{
	// Make sure the first row is not before the beginning of the screen.
	return rs < 0 ? 0 : rs;
}
static uint8_t get_row2(const int16_t re)
{
	// Make sure last row is not past the end of the screen.
	const uint16_t h = jbxvt_get_char_size().h;
	return re >= h ? h - 1 : re;
}
static void change(xcb_connection_t * restrict xc, struct JBDim * se,
	struct JBDim * ose)
{
	int16_t rs = 0, cs = 0, re = 0, ce = 0, n = jbxvt_selcmp(se, ose);
	if (!n) // nothing selected
		return;
	// repaint the start.
	{ // nn scope
		const bool nn = n < 0;
		jbxvt_selend_to_rc(&rs, &cs, nn ? se : ose);
		jbxvt_selend_to_rc(&re, &ce, nn ? ose : se);
	}
	// Invert the changed area
	invert(xc, rs, re, cs, ce, get_row1(rs), get_row2(re));
}
/*  Repaint the displayed selection to reflect the new value.
    ose1 and ose2 are assumed to represent the currently
    displayed selection endpoints.  */
void jbxvt_change_selection(xcb_connection_t * xc,
	struct JBDim * restrict ose0, struct JBDim * restrict ose1)
{
	LOG("jbxvt_change_selection()");
	if (jbxvt_selcmp(ose0, ose1) > 0) {
		struct JBDim * se = ose0;
		ose0 = ose1;
		ose1 = se;
	}
	struct JBDim * e = jbxvt_order_selection_ends(
		jbxvt_get_selection_end_points());
	change(xc, e, ose0);
	change(xc, e + 1, ose1);
}
