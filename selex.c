/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selex.h"

#include "change_selection.h"
#include "jbxvt.h"
#include "screen.h"

#define SE jbxvt.sel.end

static void handle_drag(const struct JBDim rc)
{
	//  Anchor the selection end.
	SE[0] = SE[2];
	rc_to_selend(rc.row, rc.col, &SE[1]);
	adjust_selection(&SE[1]);
}

//  Extend the selection.
void jbxvt_extend_selection(const struct JBDim point, const bool drag)
{
	struct JBDim * e = SE;
	if (jbxvt.sel.type == JBXVT_SEL_NONE)
		return; // no selection
#define F jbxvt.X.f.size
	struct JBDim rc = jbxvt_get_char_size(point);
	fix_rc(&rc);
	// Save current end points:
	struct JBDim s[] = {e[0], e[1]};
	if (drag)
		  handle_drag(rc);
	change_selection(s, s+1);
}

