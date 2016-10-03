/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selex.h"

#include "change_selection.h"
#include "jbxvt.h"
#include "screen.h"

static void handle_drag(const int16_t row, const int16_t col)
{
	//  Anchor the selection end.
	jbxvt.sel.end[0] = jbxvt.sel.anchor;
	rc_to_selend(row,col,&jbxvt.sel.end[1]);
	adjust_selection(&jbxvt.sel.end[1]);
}

//  Extend the selection.
void scr_extend_selection(const struct JBDim p, const bool drag)
{
	SelEnd * e = jbxvt.sel.end;
	if (e->type == NOSEL)
		return;
#define F jbxvt.X.f.size
	struct JBDim rc = jbxvt_get_char_size(p);
	fix_rc(&rc);
	// Save current end points:
	SelEnd s[] = {*e, *(e+1)};
	if (drag)
		  handle_drag(rc.y, rc.x);
	change_selection(s, s+1);
}

