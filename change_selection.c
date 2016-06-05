/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "change_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "screen.h"
#include "selcmp.h"
#include "selection.h"

/*  Repaint the displayed selection to reflect the new value.  ose1 and ose2
 *  are assumed to represent the currently displayed selection endpoints.  */
void change_selection(struct selst * restrict ose1,
	struct selst * restrict ose2)
{
	int16_t rs, cs, re, ce, n, row;
	struct selst *se, *se1, *se2;

	if (selcmp(ose1,ose2) > 0) {
		se = ose1;
		ose1 = ose2;
		ose2 = se;
	}
	if (selcmp(&jbxvt.sel.end1, &jbxvt.sel.end2) <= 0) {
		se1 = &jbxvt.sel.end1;
		se2 = &jbxvt.sel.end2;
	} else {
		se1 = &jbxvt.sel.end2;
		se2 = &jbxvt.sel.end1;
	}

	if ((n = selcmp(se1,ose1)) != 0) {

		/* repaint the start.
		 */
		if (n < 0) {
			selend_to_rc(&rs,&cs,se1);
			selend_to_rc(&re,&ce,ose1);
		} else {
			selend_to_rc(&rs,&cs,ose1);
			selend_to_rc(&re,&ce,se1);
		}
		uint8_t row1 = rs < 0 ? 0 : rs;
		uint8_t row2 = re >= jbxvt.scr.chars.height
			? jbxvt.scr.chars.height - 1 : re;

		/*  Invert the changed area
		 */
		for (row = row1; row <= row2; row++) {
			const int16_t y = MARGIN + row * jbxvt.X.font_height;
			const int16_t x1 = MARGIN + (row == rs
				? cs * jbxvt.X.font_width : 0);
			const int16_t x2 = MARGIN + ((row == re) ? ce
				: jbxvt.scr.chars.width)
				* jbxvt.X.font_width;
			xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.vt,
				jbxvt.X.gc.hl, 1, &(xcb_rectangle_t){
				.x = x1, .y = y, .width = x2 - x1,
				.height = jbxvt.X.font_height});
		}
	}
	if ((n = selcmp(se2,ose2)) != 0) {

		/* repaint the end.
		 */
		if (n < 0) {
			selend_to_rc(&rs,&cs,se2);
			selend_to_rc(&re,&ce,ose2);
		} else {
			selend_to_rc(&rs,&cs,ose2);
			selend_to_rc(&re,&ce,se2);
		}
		const uint8_t row1 = rs < 0 ? 0 : rs;
		const uint8_t row2 = re >= jbxvt.scr.chars.height
			? jbxvt.scr.chars.height - 1 : re;

		/*  Invert the changed area
		 */
		for (row = row1; row <= row2; row++) {
			const int16_t y = MARGIN + row * jbxvt.X.font_height;
			const int16_t x1 = MARGIN + (row == rs
				? cs * jbxvt.X.font_width : 0);
			const int16_t x2 = MARGIN + ((row == re)
				? ce : jbxvt.scr.chars.width)
				* jbxvt.X.font_width;
			xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.vt,
				jbxvt.X.gc.hl, 1, &(xcb_rectangle_t){
				.x = x1, .y = y, .width = x2 - x1,
				.height = jbxvt.X.font_height});

		}
	}
}


