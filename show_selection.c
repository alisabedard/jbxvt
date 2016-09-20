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
		const struct JBDim p1 = jbxvt_get_pixel_size(c);
		c.col = row == end.y ? end.x : col2;
		const struct JBDim p2 = jbxvt_get_pixel_size(c);
		if (p2.x > p1.x)
			xcb_poly_fill_rectangle(jbxvt.X.xcb,
				jbxvt.X.win.vt, jbxvt.X.gc.cu, 1,
				&(xcb_rectangle_t){p1.x, p1.y, p2.x - p1.x,
				f.h});
	}
}

static struct JBDim get_s(struct JBDim s, const struct JBDim rc)
{
	if (s.y < rc.row) {
		s.y = rc.row;
		s.x = rc.col;
	}
	if (s.x < rc.col)
		s.x = rc.col;
	return s;
}

static struct JBDim get_e(struct JBDim e, const struct JBDim rc)
{
	if (e.y > rc.row) {
		e.y = rc.row;
		e.x = rc.col;
	}
	if (e.x > rc.col)
		e.x = rc.col;
	return e;
}

static struct JBDim get_rc(const int16_t row, const int16_t col)
{
	return (struct JBDim){.r = row, .col = col};
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
	struct JBDim p[2];
	selend_to_rc(&p->y, &p->x, &jbxvt.sel.end[0]);
	selend_to_rc(&p[1].y, &p[1].x, &jbxvt.sel.end[1]);
	++col2;
	//  Obtain initial and final endpoints for the selection.
	struct JBDim s, e; // start and end
	const bool fwd = p->y < p[1].y || (p->y == p[1].y && p->x <= p[1].x);
	s = get_s(p[fwd?0:1], get_rc(row1, col1));
	e = get_e(p[fwd?1:0], get_rc(row2, col2));
	if (s.y > e.y)
		return;
	paint_rvid(s, e, col1, col2);
}

