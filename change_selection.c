/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "change_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "screen.h"
#include "selend.h"
#include "selection.h"

static void invert(const Size c, const Size f, const int16_t rs,
	const int16_t re, const int16_t cs, const int16_t ce,
	const uint8_t row1, const uint8_t row2, const uint8_t m)
{
	for (uint8_t row = row1; row <= row2; row++) {
		const int16_t y = m + row * f.height;
		const int16_t x1 = m + (row == rs ? cs * f.w : 0);
		const int16_t x2 = m + (row == re ? ce : c.w) * f.w;
		xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.vt,
			jbxvt.X.gc.cu, 1, &(xcb_rectangle_t){ .x = x1,
			.y = y, .width = x2 - x1, .height = f.h});
	}
}

static void change(const Size c, const Size f, const uint8_t m,
	SelEnd * se, SelEnd * ose)
{
	int16_t rs, cs, re, ce, n;
	n = selcmp(se, ose);
	if (!n) return;
	// repaint the start.
	const bool nn = n < 0;
	selend_to_rc(&rs, &cs, nn ? se : ose);
	selend_to_rc(&re, &ce, nn ? ose : se);
	uint8_t row1 = rs < 0 ? 0 : rs;
	uint8_t row2 = re >= c.h ? c.height - 1 : re;
	//  Invert the changed area
	invert(c, f, rs, re, cs, ce, row1, row2, m);
}

/*  Repaint the displayed selection to reflect the new value.  ose1 and ose2
 *  are assumed to represent the currently displayed selection endpoints.  */
void change_selection(SelEnd * restrict ose1, SelEnd * restrict ose2)
{
	SelEnd *se, *se1, *se2;

	if (selcmp(ose1, ose2) > 0) {
		se = ose1;
		ose1 = ose2;
		ose2 = se;
	}
	const bool fw = selcmp(&jbxvt.sel.end[0], &jbxvt.sel.end[1]) <= 0;
	*(fw ? &se1 : &se2) = &jbxvt.sel.end[0];
	*(fw ? &se2 : &se1) = &jbxvt.sel.end[1];
	const Size f = jbxvt.X.f.size;
	const Size c = jbxvt.scr.chars;
	const uint8_t m = MARGIN;
	change(c, f, m, se1, ose1);
	change(c, f, m, se2, ose2);
}
