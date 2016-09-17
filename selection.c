/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selection.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "save_selection.h"
#include "screen.h"
#include "show_selection.h"

#define FSZ jbxvt.X.f.size
#define CSZ jbxvt.scr.chars
#define XC jbxvt.X.xcb
#define VT jbxvt.X.win.vt

static void prop(const xcb_atom_t a)
{
	xcb_change_property(XC, XCB_PROP_MODE_REPLACE,
		VT, a, XCB_ATOM_STRING, 8, jbxvt.sel.length,
		jbxvt.sel.text);
}

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(void)
{
	LOG("scr_make_selection");
	save_selection();
	/* Set all properties which may possibly be requested.  */
	prop(XCB_ATOM_PRIMARY);
	prop(XCB_ATOM_SECONDARY);
	prop(jbxvt.X.clipboard);
	xcb_set_selection_owner(XC, VT, XCB_ATOM_PRIMARY,
		XCB_CURRENT_TIME);
}

//  Respond to a request for our current selection.
void scr_send_selection(const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property)
{
	LOG("scr_send_selection, %d, %d, %d, %d", (int)time, requestor,
		target, property);
	xcb_selection_notify_event_t e = {
		.response_type = XCB_SELECTION_NOTIFY,
		.selection = XCB_ATOM_PRIMARY, .target = target,
		.requestor = requestor, .time = time, .property
			= property == XCB_NONE
			? target : property}; // per ICCCM
	xcb_change_property(XC, XCB_PROP_MODE_REPLACE, requestor,
		property, target, 8, jbxvt.sel.length,
		jbxvt.sel.text);
	xcb_flush(XC);
	xcb_send_event(XC, true, requestor,
		XCB_SELECTION_NOTIFY, (char *)&e);
	xcb_flush(XC);
}

//  Clear the current selection.
void scr_clear_selection(void)
{
	if (!jbxvt.sel.length)
		return; // avoid double-free
	LOG("scr_clear_selection");
	jbxvt.sel.length = 0;
	show_selection(0, CSZ.h - 1, 0, CSZ.w - 1);
	jbxvt.sel.end[0].type = jbxvt.sel.end[1].type = NOSEL;
	if (jbxvt.sel.text) // avoid double-free
		free(jbxvt.sel.text);
	jbxvt.sel.text = NULL;
}

//  start a selection using the specified unit.
void scr_start_selection(struct JBDim p, enum selunit unit)
{
	LOG("scr_start_selection");
	show_selection(0, CSZ.h - 1, 0, CSZ.w - 1);
	struct JBDim rc = get_c(p);
	jbxvt.sel.unit = unit;
	fix_rc(&rc);
	rc_to_selend(rc.y, rc.x, &jbxvt.sel.anchor);
	jbxvt.sel.end[1] = jbxvt.sel.end[0] = jbxvt.sel.anchor;
	adjust_selection(&jbxvt.sel.end[1]);
	show_selection(0, CSZ.h - 1, 0, CSZ.w - 1);
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
		jb_swap(&r1, &r2);
	if (row2 < r1 || row1 > r2)
		return;
	show_selection(0, CSZ.h - 1, 0, CSZ.w - 1);
	jbxvt.sel.end[1].type = NOSEL;
}

