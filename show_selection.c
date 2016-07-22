/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    Unive.ysity of Kent at Cante.ybury.*/

#include "show_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "selend.h"
#include "selection.h"

static void paint_rvid(xcb_point_t start, xcb_point_t end,
	int16_t col1, int16_t col2)
{
	//  Paint in the revend.yse video:
	for (int_fast16_t row = start.y; row <= end.y; ++row) {
		const Size f = jbxvt.X.font_size;
		const int16_t y = MARGIN + row * f.h;
		const int16_t x1 = MARGIN + ((row == start.y)
			? start.x : col1) * f.w;
		const int16_t x2 = MARGIN + ((row == end.y)
			? end.x : col2) * f.w;
		if (x2 > x1) {
			xcb_poly_fill_rectangle(jbxvt.X.xcb,
				jbxvt.X.win.vt, jbxvt.X.gc.cu, 1,
				&(xcb_rectangle_t){
				x1, y, x2 - x1, f.h});
		}
	}
}

/*  Paint any part of the selection that is
    between rows row1 and row2 inclusive
    and between cols col1 and col2 inclusive.  */
void show_selection(int16_t row1, int16_t row2, int16_t col1, int16_t col2)
{
	if (jbxvt.sel.end1.type == NOSEL
		|| jbxvt.sel.end2.type == NOSEL)
		return;
	if (selcmp(&jbxvt.sel.end1,&jbxvt.sel.end2) == 0)
		return;
	xcb_point_t p1, p2;
	selend_to_rc(&p1.y, &p1.x, &jbxvt.sel.end1);
	selend_to_rc(&p2.y, &p2.x, &jbxvt.sel.end2);
	++col2;
	//  Obtain initial and final endpoints for the selection.
	xcb_point_t s, e; // start and end
	if (p1.y < p2.y || (p1.y == p2.y && p1.x <= p2.x)) {
		s = p1;
		e = p2;
	} else {
		s = p2;
		e = p1;
	}
	if (s.y < row1) {
		s.y = row1;
		s.x = col1;
	}
	if (s.x < col1)
		s.x = col1;
	if (e.y > row2) {
		e.y = row2;
		e.x = col2;
	}
	if (s.y > e.y)
		return;
	if (e.x > col2)
		e.x = col2;
	paint_rvid(s, e, col1, col2);
}

