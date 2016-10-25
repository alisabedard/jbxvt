/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "selection.h"
#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "save_selection.h"
#include "selend.h"
#include "screen.h"
#include "show_selection.h"
#include "window.h"
#define SE jbxvt.sel.end
static inline void prop(const xcb_atom_t a)
{
	jbxvt_set_property(a, jbxvt.sel.length, jbxvt.sel.text);
}
//  Make the selection currently delimited by the selection end markers.
void jbxvt_make_selection(void)
{
	LOG("jbxvt_make_selection");
	jbxvt_save_selection();
	/* Set all properties which may possibly be requested.  */
	prop(XCB_ATOM_PRIMARY);
	prop(XCB_ATOM_SECONDARY);
	prop(jbxvt.X.clipboard);
	xcb_set_selection_owner(jbxvt.X.xcb, jbxvt.X.win.main,
		XCB_ATOM_PRIMARY, XCB_CURRENT_TIME);
}
//  Respond to a request for our current selection.
void jbxvt_send_selection(const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property)
{
	LOG("jbxvt_send_selection, %d, %d, %d, %d", (int)time, requestor,
		target, property);
	xcb_selection_notify_event_t e = {
		.response_type = XCB_SELECTION_NOTIFY,
		.selection = XCB_ATOM_PRIMARY, .target = target,
		.requestor = requestor, .time = time, .property
			= property == XCB_NONE
			? target : property}; // per ICCCM
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE, requestor,
		property, target, 8, jbxvt.sel.length,
		jbxvt.sel.text);
	xcb_flush(jbxvt.X.xcb);
	xcb_send_event(jbxvt.X.xcb, true, requestor,
		XCB_SELECTION_NOTIFY, (char *)&e);
	xcb_flush(jbxvt.X.xcb);
}
//  Clear the current selection.
void jbxvt_clear_selection(void)
{
	if (jbxvt.sel.text)
		free(jbxvt.sel.text);
	jbxvt.sel.text = NULL;
	jbxvt.sel.type = JBXVT_SEL_NONE;
}
//  start a selection using the specified unit.
void jbxvt_start_selection(struct JBDim p, enum JBXVTSelectionUnit unit)
{
	jbxvt_show_selection(); // clear previous
	jbxvt_clear_selection(); // free previous selection
	p = jbxvt_get_char_size(p);
	jbxvt.sel.unit = unit;
	jbxvt_rc_to_selend(p.y, p.x, &SE[2]);
	SE[0] = SE[1] = SE[2];
	jbxvt_adjust_selection(&SE[1]);
	jbxvt_show_selection();
}
/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.  */
void jbxvt_check_selection(const int16_t row1, const int16_t row2)
{
	if (jbxvt.sel.type != JBXVT_SEL_ON_SCREEN)
		return;
	int16_t r1 = SE[0].index, r2 = SE[1].index;
	if (r1 > r2)
		JB_SWAP(int16_t, r1, r2);
	if (row2 < r1 || row1 > r2)
		return;
	jbxvt_show_selection();
	jbxvt.sel.type = JBXVT_SEL_NONE;
}
