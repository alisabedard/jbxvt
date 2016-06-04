/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "show_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "selcmp.h"
#include "selection.h"
#include "selst.h"

/*  Paint any part of the selection that is between rows row1 and row2 inclusive
 *  and between cols col1 and col2 inclusive.
 */
void show_selection(int16_t row1, int16_t row2, int16_t col1, int16_t col2)
{
	if (jbxvt.sel.end1.se_type == NOSEL
		|| jbxvt.sel.end2.se_type == NOSEL)
		return;
	if (selcmp(&jbxvt.sel.end1,&jbxvt.sel.end2) == 0)
		return;
	int16_t r1, c1, r2, c2;
	selend_to_rc(&r1,&c1,&jbxvt.sel.end1);
	selend_to_rc(&r2,&c2,&jbxvt.sel.end2);
	col2++;

	//  Obtain initial and final endpoints for the selection.
	int16_t sr, sc, er, ec;
	if (r1 < r2 || (r1 == r2 && c1 <= c2)) {
		sr = r1;
		sc = c1;
		er = r2;
		ec = c2;
	} else {
		sr = r2;
		sc = c2;
		er = r1;
		ec = c1;
	}
	if (sr < row1) {
		sr = row1;
		sc = col1;
	}
	if (sc < col1)
		sc = col1;
	if (er > row2) {
		er = row2;
		ec = col2;
	}
	if (ec > col2)
		ec = col2;

	if (sr > er)
		return;
	//  Paint in the reverse video:
	for (int16_t row = sr; row <= er; ++row) {
		const int16_t y = MARGIN + row * jbxvt.X.font_height;
		const int16_t x1 = MARGIN + ((row == sr) ? sc : col1)
			* jbxvt.X.font_width;
		const int16_t x2 = MARGIN + ((row == er) ? ec : col2)
			* jbxvt.X.font_width;
		if (x2 > x1) {
#ifdef USE_XCB
			xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.vt,
				XCBGC(jbxvt.X.gc.hl), 1, &(xcb_rectangle_t){
				x1, y, x2 - x1, jbxvt.X.font_height});
#else//!USE_XCB
			XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.hl,
				x1,y,x2 - x1,jbxvt.X.font_height);
#endif//USE_XCB
		}
	}
}

