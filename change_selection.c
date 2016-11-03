/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "change_selection.h"
#include "config.h"
#include "cursor.h"
#include "font.h"
#include "jbxvt.h"
#include "screen.h"
#include "selend.h"
#include "selection.h"
#include "window.h"
#define FSZ jbxvt_get_font_size()
static void invert(xcb_connection_t * xc, const int16_t rs,
	const int16_t re, const int16_t cs, const int16_t ce,
	const uint8_t row1, const uint8_t row2)
{
	for (uint8_t row = row1; row <= row2; row++) {
		const int16_t y = row * FSZ.height;
		const int16_t x1 = row == rs ? cs * FSZ.w : 0;
		const int16_t x2 = (row == re ? ce : jbxvt.scr.chars.w) * FSZ.w;
		xcb_poly_fill_rectangle(xc, jbxvt_get_vt_window(xc),
			jbxvt_get_cursor_gc(xc), 1, &(xcb_rectangle_t){
			.x = x1, .y = y, .width = x2 - x1, .height = FSZ.h});
	}
}
static void change(xcb_connection_t * xc, struct JBDim * se,
	struct JBDim * ose)
{
	int16_t rs = 0, cs = 0, re = 0, ce = 0, n;
	n = jbxvt_selcmp(se, ose);
	if (!n) return;
	// repaint the start.
	const bool nn = n < 0;
	jbxvt_selend_to_rc(&rs, &cs, nn ? se : ose);
	jbxvt_selend_to_rc(&re, &ce, nn ? ose : se);
	uint8_t row1 = rs < 0 ? 0 : rs;
	uint8_t row2 = re >= jbxvt.scr.chars.h ? jbxvt.scr.chars.h - 1 : re;
	//  Invert the changed area
	invert(xc, rs, re, cs, ce, row1, row2);
}
/*  Repaint the displayed selection to reflect the new value.  ose1 and ose2
 *  are assumed to represent the currently displayed selection endpoints.  */
void jbxvt_change_selection(xcb_connection_t * xc,
	struct JBDim * restrict ose0, struct JBDim * restrict ose1)
{
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
