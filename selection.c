/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selection.h"

#include "config.h"
#include "jbxvt.h"
#include "save_selection.h"
#include "screen.h"
#include "show_selection.h"

static void prop(const xcb_window_t win, const xcb_atom_t a)
{
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		win, a, XCB_ATOM_STRING, 8, jbxvt.sel.length, jbxvt.sel.text);
}

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(void)
{
	save_selection();
	/* Set all properties which may possibly be requested.  */
	const xcb_window_t v = jbxvt.X.win.vt;
	prop(v, XCB_ATOM_PRIMARY);
	prop(v, XCB_ATOM_SECONDARY);
	prop(v, jbxvt.X.clipboard);
	prop(jbxvt.X.screen->root, XCB_ATOM_CUT_BUFFER0);
	xcb_set_selection_owner(jbxvt.X.xcb, v, XCB_ATOM_PRIMARY,
		XCB_CURRENT_TIME);
}

//  Respond to a request for our current selection.
void scr_send_selection(const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property)
{
	// x events must be 32 bytes long:
	xcb_selection_notify_event_t e = {
		.response_type = XCB_SELECTION_NOTIFY,
		.selection = XCB_ATOM_PRIMARY, .target = target,
		.requestor = requestor, .time = time, .property
			= property ? property : target}; // per ICCCM
	// If property is None, use taget, per ICCCM
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE, requestor,
		property == XCB_NONE ? target : property,
		XCB_ATOM_STRING, 8, jbxvt.sel.length,
		jbxvt.sel.text);
	xcb_send_event(jbxvt.X.xcb, false, requestor,
		XCB_SELECTION_NOTIFY, (char *)&e);
	// send it before e looses scope
	xcb_flush(jbxvt.X.xcb);
}

//  Clear the current selection.
void scr_clear_selection(void)
{
	if (jbxvt.sel.text)
		jbxvt.sel.length = 0;
	const Size c = jbxvt.scr.chars;
	show_selection(0, c.h - 1, 0, c.w - 1);
	jbxvt.sel.end[0].type = jbxvt.sel.end[1].type = NOSEL;
}

//  start a selection using the specified unit.
void scr_start_selection(xcb_point_t p, enum selunit unit)
{
	const Size c = jbxvt.scr.chars;
	const Size f = jbxvt.X.f.size;
	show_selection(0, c.h - 1, 0, c.w - 1);
	xcb_point_t rc = { .x = (p.x - MARGIN) / f.w,
		.y = (p.y - MARGIN) / f.h};
	jbxvt.sel.unit = unit;
	fix_rc(&rc);
	rc_to_selend(rc.y, rc.x, &jbxvt.sel.anchor);
	jbxvt.sel.end[1] = jbxvt.sel.end[0] = jbxvt.sel.anchor;
	adjust_selection(&jbxvt.sel.end[1]);
	show_selection(0, c.h - 1, 0, c.w - 1);
}

static int16_t row(SelEnd * restrict e)
{
	return e->type == SCREENSEL ? e->index : -1;
}

/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.  */
void check_selection(const int16_t row1, const int16_t row2)
{
	SelEnd *e1 = &jbxvt.sel.end[0], *e2 = &jbxvt.sel.end[1];
	if (e1->type == NOSEL || e2->type == NOSEL)
		return;
	int16_t r1 = row(e1), r2 = row(e2);
	if (r1 > r2)
		SWAP(int16_t, r1, r2);
	if (row2 < r1 || row1 > r2)
		return;
	const Size c = jbxvt.scr.chars;
	show_selection(0, c.h - 1, 0, c.w - 1);
	jbxvt.sel.end[1].type = NOSEL;
}

