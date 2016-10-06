/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "change_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "screen.h"
#include "selend.h"
#include "selection.h"

#define FSZ jbxvt.X.f.size

static void invert(const int16_t rs, const int16_t re, const int16_t cs,
	const int16_t ce, const uint8_t row1, const uint8_t row2)
{
	for (uint8_t row = row1; row <= row2; row++) {
		const int16_t y = row * FSZ.height;
		const int16_t x1 = row == rs ? cs * FSZ.w : 0;
		const int16_t x2 = (row == re ? ce : jbxvt.scr.chars.w) * FSZ.w;
		xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.vt,
			jbxvt.X.gc.cu, 1, &(xcb_rectangle_t){ .x = x1,
			.y = y, .width = x2 - x1, .height = FSZ.h});
	}
}

static void change(struct JBDim * se, struct JBDim * ose)
{
	int16_t rs = 0, cs = 0, re = 0, ce = 0, n;
	n = selcmp(se, ose);
	if (!n) return;
	// repaint the start.
	const bool nn = n < 0;
	selend_to_rc(&rs, &cs, nn ? se : ose);
	selend_to_rc(&re, &ce, nn ? ose : se);
	uint8_t row1 = rs < 0 ? 0 : rs;
	uint8_t row2 = re >= jbxvt.scr.chars.h ? jbxvt.scr.chars.h - 1 : re;
	//  Invert the changed area
	invert(rs, re, cs, ce, row1, row2);
}

/*  Repaint the displayed selection to reflect the new value.  ose1 and ose2
 *  are assumed to represent the currently displayed selection endpoints.  */
void change_selection(struct JBDim * restrict ose1, struct JBDim * restrict ose2)
{
	struct JBDim *se, *se1, *se2;

	if (selcmp(ose1, ose2) > 0) {
		se = ose1;
		ose1 = ose2;
		ose2 = se;
	}
	const bool fw = selcmp(&jbxvt.sel.end[0], &jbxvt.sel.end[1]) <= 0;
	*(fw ? &se1 : &se2) = &jbxvt.sel.end[0];
	*(fw ? &se2 : &se1) = &jbxvt.sel.end[1];
	change(se1, ose1);
	change(se2, ose2);
}
