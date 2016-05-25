/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_extend_selection.h"

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
	jbxvt.sel.end1 = jbxvt.sel.anchor;
	rc_to_selend(row,col,&jbxvt.sel.end2);
	adjust_selection(&jbxvt.sel.end2);
}

static struct selst * get_nearest_endpoint(const int16_t row,
	const int16_t col)
{
	int16_t r1, r2, c1, c2;
	selend_to_rc(&r1,&c1,&jbxvt.sel.end1);
	selend_to_rc(&r2,&c2,&jbxvt.sel.end2);

	//  Determine which is the nearest endpoint.
	if (abs(r1 - row) < abs(r2 - row))
		  return &jbxvt.sel.end1;
	else if (abs(r2 - row) < abs(r1 - row))
		  return &jbxvt.sel.end2;
	else if (r1 == r2) {
		if (row < r1)
			  return (c1 < c2) ? &jbxvt.sel.end1
				  : &jbxvt.sel.end2;
		else if (row > r1)
			  return (c1 > c2) ? &jbxvt.sel.end1
				  : &jbxvt.sel.end2;
		else
			  return abs(c1 - col) < abs(c2 - col)
				  ? &jbxvt.sel.end1 : &jbxvt.sel.end2;
	} else
		  return &jbxvt.sel.end2;
}

//  Extend the selection.
void scr_extend_selection(const Dim p, const bool drag)
{
	if (jbxvt.sel.end1.se_type == NOSEL)
		return;
	Dim rc = { .col = (p.x - MARGIN) / jbxvt.X.font_width,
		.row = (p.y - MARGIN) / jbxvt.X.font_height };
	fix_rc(&rc);
	// Save current end points:
	struct selst sesave1 = jbxvt.sel.end1;
	struct selst sesave2 = jbxvt.sel.end2;

	if (drag) 
		  handle_drag(rc.r, rc.c);
	else {
		struct selst * se = get_nearest_endpoint(rc.r, rc.c);
		rc_to_selend(rc.r, rc.c, se);
		adjust_selection(se);
	}

	change_selection(&sesave1,&sesave2);
}

