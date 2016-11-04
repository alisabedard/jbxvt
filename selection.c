/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "selection.h"
#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/xcb.h"
#include "libjb/util.h"
#include "save_selection.h"
#include "selend.h"
#include "screen.h"
#include "show_selection.h"
#include "size.h"
#include "window.h"
static struct JBXVTSelectionData selection_data;
#define SE selection_data.end
// Return selection end points: first, second, and anchor
struct JBDim * jbxvt_get_selection_end_points(void)
{
	return selection_data.end;
}
enum JBXVTSelectionUnit jbxvt_get_selection_unit(void)
{
	return selection_data.unit;
}
bool jbxvt_is_selected(void)
{
	return selection_data.on_screen;
}
//  Return the atom corresponding to "CLIPBOARD"
xcb_atom_t jbxvt_get_clipboard(xcb_connection_t * xc)
{
	static xcb_atom_t a;
	if (a)
		return a;
	return a = jb_get_atom(xc, "CLIPBOARD");
}
static inline void prop(xcb_connection_t * xc, const xcb_atom_t a)
{
	jbxvt_set_property(xc, a, selection_data.length, selection_data.text);
}
//  Make the selection currently delimited by the selection end markers.
void jbxvt_make_selection(xcb_connection_t * xc)
{
	LOG("jbxvt_make_selection");
	jbxvt_save_selection(&selection_data);
	/* Set all properties which may possibly be requested.  */
	prop(xc, XCB_ATOM_PRIMARY);
	prop(xc, XCB_ATOM_SECONDARY);
	prop(xc, jbxvt_get_clipboard(xc));
	xcb_set_selection_owner(xc, jbxvt_get_main_window(xc),
		XCB_ATOM_PRIMARY, XCB_CURRENT_TIME);
}
//  Respond to a request for our current selection.
void jbxvt_send_selection(xcb_connection_t * xc,
	const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property)
{
	LOG("jbxvt_send_selection, %d, %d, %d, %d", (int)time,
		requestor, target, property);
	xcb_selection_notify_event_t e = {
		.response_type = XCB_SELECTION_NOTIFY,
		.selection = XCB_ATOM_PRIMARY, .target = target,
		.requestor = requestor, .time = time, .property
			= property == XCB_NONE
			? target : property}; // per ICCCM
	xcb_change_property(xc, XCB_PROP_MODE_REPLACE, requestor,
		property, target, 8, selection_data.length, selection_data.text);
	xcb_flush(xc);
	xcb_send_event(xc, true, requestor,
		XCB_SELECTION_NOTIFY, (char *)&e);
	xcb_flush(xc);
}
//  Clear the current selection.
void jbxvt_clear_selection(void)
{
	if (selection_data.text)
		free(selection_data.text);
	selection_data.text = NULL;
	selection_data.on_screen = false;
}
//  start a selection using the specified unit.
void jbxvt_start_selection(xcb_connection_t * xc,
	struct JBDim p, enum JBXVTSelectionUnit unit)
{
	jbxvt_show_selection(xc); // clear previous
	jbxvt_clear_selection(); // free previous selection
	p = jbxvt_pixels_to_chars(p);
	jbxvt_rc_to_selend(p.y, p.x, &SE[2]);
	selection_data.unit = unit;
	selection_data.on_screen = true;
	SE[0] = SE[1] = SE[2];
	jbxvt_adjust_selection(&SE[1]);
	jbxvt_show_selection(xc);
}
/*  Determine if the current selection overlaps row1 through row2.
    If it does then remove it from the screen.  */
void jbxvt_check_selection(xcb_connection_t * xc,
	const int16_t row1, const int16_t row2)
{
	if (!selection_data.on_screen)
		return;
	int16_t r1 = SE[0].index, r2 = SE[1].index;
	if (r1 > r2)
		JB_SWAP(int16_t, r1, r2);
	if (row2 < r1 || row1 > r2)
		return;
	jbxvt_show_selection(xc);
	selection_data.on_screen = false;
}
