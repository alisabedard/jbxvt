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

//  Extend the selection.
void scr_extend_selection(const xcb_point_t p, const bool drag)
{
	SelEnd * e = jbxvt.sel.end;
	if (e->type == NOSEL)
		return;
	const Size f = jbxvt.X.f.size;
	xcb_point_t rc = { .x = (p.x - MARGIN) / f.w,
		.y = (p.y - MARGIN) / f.h};
	fix_rc(&rc);
	// Save current end points:
	SelEnd s[] = {*e, *(e+1)};
	if (drag)
		  handle_drag(rc.y, rc.x);
	change_selection(s, s+1);
}

