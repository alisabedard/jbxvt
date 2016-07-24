/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selex.h"

#include "config.h"
#include "change_selection.h"
#include "jbxvt.h"
#include "screen.h"
#include "selection.h"
#include "show_selection.h"

#include <stdlib.h>

static void handle_drag(const int16_t row, const int16_t col)
{
	//  Anchor the selection end.
	jbxvt.sel.end[0] = jbxvt.sel.anchor;
	rc_to_selend(row,col,&jbxvt.sel.end[1]);
	adjust_selection(&jbxvt.sel.end[1]);
}

static SelEnd * get_nearest_endpoint(const int16_t row,
	const int16_t col)
{
	int16_t r1, r2, c1, c2;
	selend_to_rc(&r1,&c1,&jbxvt.sel.end[0]);
	selend_to_rc(&r2,&c2,&jbxvt.sel.end[1]);

	//  Determine which is the nearest endpoint.
	if (abs(r1 - row) < abs(r2 - row))
		  return &jbxvt.sel.end[0];
	else if (abs(r2 - row) < abs(r1 - row))
		  return &jbxvt.sel.end[1];
	else if (r1 == r2) {
		if (row < r1)
			  return (c1 < c2) ? &jbxvt.sel.end[0]
				  : &jbxvt.sel.end[1];
		else if (row > r1)
			  return (c1 > c2) ? &jbxvt.sel.end[0]
				  : &jbxvt.sel.end[1];
		return abs(c1 - col) < abs(c2 - col)
			? &jbxvt.sel.end[0] : &jbxvt.sel.end[1];
	}
	return &jbxvt.sel.end[1];
}

//  Extend the selection.
void scr_extend_selection(const xcb_point_t p, const bool drag)
{
	if (jbxvt.sel.end[0].type == NOSEL)
		return;
	const Size f = jbxvt.X.font_size;
	xcb_point_t rc = { .x = (p.x - MARGIN) / f.w,
		.y = (p.y - MARGIN) / f.h};
	fix_rc(&rc);
	// Save current end points:
	SelEnd sesave1 = jbxvt.sel.end[0];
	SelEnd sesave2 = jbxvt.sel.end[1];

	if (drag)
		  handle_drag(rc.y, rc.x);
	else {
		SelEnd * se = get_nearest_endpoint(rc.y, rc.x);
		rc_to_selend(rc.y, rc.x, se);
		adjust_selection(se);
	}

	change_selection(&sesave1, &sesave2);
}

