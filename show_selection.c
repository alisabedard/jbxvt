/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury. */
#include "show_selection.h"
#include "jbxvt.h"
#include "screen.h"
#include "selend.h"
static void paint_rvid(struct JBDim start, struct JBDim end,
	int16_t col1, int16_t col2)
{
	//  Paint in the reverse video:
	for (int_fast16_t row = start.y; row <= end.y; ++row) {
		struct JBDim c = {.row = row, .col = row == start.y
			? start.x : col1};
		const struct JBDim p1 = jbxvt_get_pixel_size(c);
		c.col = row == end.y ? end.x : col2;
		const struct JBDim p2 = jbxvt_get_pixel_size(c);
		if (p2.x <= p1.x)
			continue;
		xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.vt,
			jbxvt.X.gc.cu, 1, &(xcb_rectangle_t){p1.x, p1.y,
			p2.x - p1.x, jbxvt.X.font.size.h});
	}
}
// Paint the selection on screen
void jbxvt_show_selection(void)
{
	if (jbxvt.sel.type == JBXVT_SEL_NONE
		|| jbxvt_selcmp(&jbxvt.sel.end[0], &jbxvt.sel.end[1]) == 0)
		return;
	struct JBDim p[2] = {};
	jbxvt_selend_to_rc(&p->y, &p->x, &jbxvt.sel.end[0]);
	jbxvt_selend_to_rc(&p[1].y, &p[1].x, &jbxvt.sel.end[1]);
	struct JBDim r[] = {{}, jbxvt.scr.chars};
	//  Obtain initial and final endpoints for the selection.
	const bool fwd = p->y < p[1].y || (p->y == p[1].y && p->x <= p[1].x);
	paint_rvid(p[fwd?0:1], p[fwd?1:0], r[0].x, r[1].x);
}
