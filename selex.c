/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "selex.h"
#include "change_selection.h"
#include "libjb/util.h"
#include "selection.h"
#include "selend.h"
#include "size.h"
static void handle_drag(const struct JBDim rc)
{
	struct JBDim * e = jbxvt_get_selection_end_points();
	//  Anchor the selection end.
	e[0] = e[2];
	jbxvt_rc_to_selend(rc.row, rc.col, &e[1]);
	jbxvt_adjust_selection(&e[1]);
}
static void save_current_end_points(struct JBDim * restrict s,
	struct JBDim point)
{
	struct JBDim * e = jbxvt_get_selection_end_points();
	s[0] = e[0];
	s[1] = e[1];
	s[2] = point;
}
// Get a char-sized JBDim structure fitting within the screen
static struct JBDim jbxvt_get_constrained(struct JBDim rc)
{
	struct JBDim c = jbxvt_get_char_size();
	if (c.w) {
		JB_LIMIT(rc.x, c.w - 1, 0);
	} else
		rc.x = 0;
	if (c.h) {
		JB_LIMIT(rc.y, c.h - 1, 0);
	} else
		rc.y = 0;
	return rc;
}
//  Extend the selection.
void jbxvt_extend_selection(xcb_connection_t * xc,
	const struct JBDim point, const bool drag)
{
	if (!jbxvt_is_selected())
		return; // no selection
	struct JBDim s[3];
	save_current_end_points(s,
		jbxvt_get_constrained(jbxvt_pixels_to_chars(point)));
	if (drag)
		  handle_drag(s[2]);
	jbxvt_change_selection(xc, s, s + 1);
}
