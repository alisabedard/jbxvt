/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "change_selection.h"
#include "font.h"
#include "gc.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "selection.h"
#include "selend.h"
#include "size.h"
#include "window.h"
static void invert(xcb_connection_t * xc, const int16_t rs,
	const int16_t re, const int16_t cs, const int16_t ce,
	const uint8_t row1, const uint8_t row2)
{
	const struct JBDim csz = jbxvt_get_char_size(),
	      fsz = jbxvt_get_font_size();
	for (uint8_t row = row1; row <= row2; row++) {
		const int16_t x[] = {row == rs ? cs * fsz.w : 0,
			(row == re ? ce : csz.w) * fsz.w};
		xcb_poly_fill_rectangle(xc, jbxvt_get_vt_window(xc),
			jbxvt_get_cursor_gc(xc), 1, &(xcb_rectangle_t){
			.x = x[0], .y = row * fsz.height,
			.width = x[1] - x[0], .height = fsz.h});
	}
}
// Make sure the first row is not before the beginning of the screen.
static inline uint8_t get_start_row(const int16_t rs)
{
	return rs < 0 ? 0 : rs;
}
// Make sure last row is not past the end of the screen.
static inline uint8_t get_end_row(const int16_t re)
{
	const uint16_t h = jbxvt_get_char_size().h;
	return re >= h ? h - 1 : re;
}
static void change(xcb_connection_t * restrict xc, struct JBDim * se,
	struct JBDim * ose)
{
	const int16_t n = jbxvt_selcmp(se, ose);
	if (!n) // nothing selected
		return;
	// repaint the start.
	struct JBDim start, end;
	{ // nn scope
		const bool nn = n < 0;
		start = jbxvt_get_selend_position(nn ? se : ose);
		end = jbxvt_get_selend_position(nn ? ose : se);
	}
	// Invert the changed area
	invert(xc, start.row, end.row, start.col, end.col,
		get_start_row(start.row), get_end_row(end.row));
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
