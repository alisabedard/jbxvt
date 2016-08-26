/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    Unive.ysity of Kent at Cante.ybury.*/

#include "show_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "screen.h"
#include "selend.h"
#include "selection.h"

static void paint_rvid(struct JBDim start, struct JBDim end,
	int16_t col1, int16_t col2)
{
	//  Paint in the revend.yse video:
	for (int_fast16_t row = start.y; row <= end.y; ++row) {
		const struct JBDim f = jbxvt.X.f.size;
		struct JBDim c = {.row = row, .col = row == start.y
			? start.x : col1};
		const struct JBDim p1 = get_p(c);
		c.col = row == end.y ? end.x : col2;
		const struct JBDim p2 = get_p(c);
		if (p2.x > p1.x)
			xcb_poly_fill_rectangle(jbxvt.X.xcb,
				jbxvt.X.win.vt, jbxvt.X.gc.cu, 1,
				&(xcb_rectangle_t){p1.x, p1.y, p2.x - p1.x,
				f.h});
	}
}

/*  Paint any part of the selection that is
    between rows row1 and row2 inclusive
    and between cols col1 and col2 inclusive.  */
void show_selection(int16_t row1, int16_t row2, int16_t col1, int16_t col2)
{
	if (jbxvt.sel.end[0].type == NOSEL
		|| jbxvt.sel.end[1].type == NOSEL)
		return;
	if (selcmp(&jbxvt.sel.end[0],&jbxvt.sel.end[1]) == 0)
		return;
	struct JBDim p1, p2;
	selend_to_rc(&p1.y, &p1.x, &jbxvt.sel.end[0]);
	selend_to_rc(&p2.y, &p2.x, &jbxvt.sel.end[1]);
	++col2;
	//  Obtain initial and final endpoints for the selection.
	struct JBDim s, e; // start and end
	const bool fwd = p1.y < p2.y || (p1.y == p2.y && p1.x <= p2.x);
	s = fwd ? p1 : p2;
	e = fwd ? p2 : p1;
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

