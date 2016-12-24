/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury. */
#include "show_selection.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <xcb/xproto.h>
#include "cursor.h"
#include "font.h"
#include "libjb/JBDim.h"
#include "selection.h"
#include "selend.h"
#include "size.h"
#include "window.h"
static void paint_rvid(xcb_connection_t * xc,
	struct JBDim start, struct JBDim end,
	int16_t col1, int16_t col2)
{
	//  Paint in the reverse video:
	for (int_fast16_t row = start.y; row <= end.y; ++row) {
		struct JBDim c = {.row = row, .col = row == start.y
			? start.x : col1};
		const struct JBDim p1 = jbxvt_chars_to_pixels(c);
		c.col = row == end.y ? end.x : col2;
		const struct JBDim p2 = jbxvt_chars_to_pixels(c);
		if (p2.x <= p1.x)
			continue;
		xcb_poly_fill_rectangle(xc, jbxvt_get_vt_window(xc),
			jbxvt_get_cursor_gc(xc), 1,
			&(xcb_rectangle_t){p1.x, p1.y,
			p2.x - p1.x, jbxvt_get_font_size().h});
	}
}
// Paint the selection on screen
void jbxvt_show_selection(xcb_connection_t * xc)
{
	struct JBDim * e = jbxvt_get_selection_end_points();
	if (!jbxvt_is_selected() || jbxvt_selcmp(&e[0], &e[1]) == 0)
		return;
	struct JBDim p[2] = {};
	jbxvt_selend_to_rc(&p->y, &p->x, &e[0]);
	jbxvt_selend_to_rc(&p[1].y, &p[1].x, &e[1]);
	struct JBDim r[] = {{}, jbxvt_get_char_size()};
	//  Obtain initial and final endpoints for the selection.
	const bool fwd = p->y < p[1].y || (p->y == p[1].y && p->x <= p[1].x);
	paint_rvid(xc, p[fwd?0:1], p[fwd?1:0], r[0].x, r[1].x);
}
