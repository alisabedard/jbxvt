/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "xevents.h"
#include "command.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "mouse.h"
#include "sbar.h"
#include "scr_move.h"
#include "selection.h"
#include "selex.h"
#include "selreq.h"
xcb_atom_t jbxvt_get_wm_del_win(void)
{
	static long unsigned int a;
	if(!a) { // Init on first call:
		xcb_connection_t * c = jbxvt.X.xcb;
		a = jb_get_atom(c, "WM_DELETE_WINDOW");
		xcb_change_property(c, XCB_PROP_MODE_REPLACE,
			jbxvt.X.win.main, jb_get_atom(c, "WM_PROTOCOLS"),
			XCB_ATOM_ATOM, 32, 1, &a);
	}
	return a;
}
static void handle_motion_notify(struct JBXVTEvent * restrict xe)
{
	const xcb_rectangle_t r = xe->box;
	const struct JBDim b = {.x = r.x, .y = r.y};
	if (xe->window == jbxvt.X.win.sb
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_2))
		jbxvt_scroll_to(b.y);
	else if (xe->window == jbxvt.X.win.vt
		&& jbxvt_get_mouse_motion_tracked())
		jbxvt_track_mouse(xe->button, xe->state, b, JBXVT_MOTION);
	else if (xe->window == jbxvt.X.win.vt
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_1)
		&& !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		if (jbxvt_get_mouse_tracked())
			  return;
		jbxvt_extend_selection(b, true);
	}
}
static void sbop(struct JBXVTEvent * restrict xe, const bool up)
{
	if (jbxvt_get_mouse_tracked()) // let the application handle scrolling
		return;
	// xterm's behavior if alternate screen in use is to move the cursor
	if (jbxvt.scr.current == jbxvt.scr.s) // first screen
		jbxvt_set_scroll(jbxvt.scr.offset + (up ? -xe->box.y
			: xe->box.y) / jbxvt.X.font.size.h);
	else
		jbxvt_move(0, up ? -1 : 1, ROW_RELATIVE | COL_RELATIVE);
}
static void handle_button_release(struct JBXVTEvent * restrict xe)
{
	if (xe->window == jbxvt.X.win.sb) {
		switch (xe->button) {
		case 1:
		case 5:
			sbop(xe, true);
			break;
		case 3:
		case 4:
			sbop(xe, false);
			break;
		}
	} else if (xe->window == jbxvt.X.win.vt
		&& jbxvt_get_mouse_tracked() && xe->button <= 3) {
		/* check less than or equal to 3, since xterm does not
		   report mouse wheel release events.  */
		jbxvt_track_mouse(xe->button, xe->state,
			(struct JBDim){.x = xe->box.x, .y = xe->box.y},
			JBXVT_RELEASE);
	} else if (xe->window == jbxvt.X.win.vt
		&& !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (xe->button) {
		case 1:
		case 3:
			jbxvt_make_selection();
			break;
		case 2:
			jbxvt_request_selection(xe->time);
			break;
		case 4:
			sbop(xe, false);
			break;
		case 5:
			sbop(xe, true);
			break;
		}
	}
}
static void handle_button1_press(struct JBXVTEvent * restrict xe,
	const struct JBDim b)
{
	static unsigned int time1, time2;
	if (xe->time - time2
		< MP_INTERVAL) {
		time1 = 0;
		time2 = 0;
		jbxvt_start_selection(b, JBXVT_SEL_UNIT_LINE);
	} else if (xe->time - time1
		< MP_INTERVAL) {
		time2 = xe->time;
		jbxvt_start_selection(b, JBXVT_SEL_UNIT_WORD);
	} else {
		time1 = xe->time;
		jbxvt_start_selection(b, JBXVT_SEL_UNIT_CHAR);
	}
}
static void handle_button_press(struct JBXVTEvent * restrict xe)
{
	const xcb_window_t v = jbxvt.X.win.vt;
	if (xe->window == v && xe->state == XCB_KEY_BUT_MASK_CONTROL) {
		jbxvt_toggle_scrollbar();
		return;
	}
	const struct JBDim b = {.x = xe->box.x, .y = xe->box.y};
	if (xe->window == v && jbxvt_get_mouse_tracked())
		jbxvt_track_mouse(xe->button, xe->state,
			(struct JBDim){.x = xe->box.x, .y = xe->box.y}, 0);
	else if (xe->window == v && !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (xe->button) {
		case 1:
			handle_button1_press(xe, b);
			return;
		case 3:
			jbxvt_extend_selection(b, false);
			return;
		}
		return;
	}
	if (xe->window == jbxvt.X.win.sb && xe->button == 2)
		jbxvt_scroll_to(xe->box.y);
}
static void handle_focus(const bool in)
{
	if (jbxvt.mode.mouse_focus_evt)
		cprintf("\033[%c]", in ? 'I' : 'O');
}
// Handle X11 event described by xe
bool jbwm_handle_xevents(struct JBXVTEvent * xe)
{
	switch (xe->type &~0x80) { // Ordered numerically:
	case 0: // Unimplemented, undefined, no event
		return false;
	// Put things to ignore here:
	case 150: // Undefined
	case XCB_KEY_RELEASE: // Unimplemented
	case XCB_NO_EXPOSURE: // Unimplemented
	case XCB_REPARENT_NOTIFY: // handle here to ensure cursor filled.
	case XCB_MAP_NOTIFY: // handle here to ensure cursor filled.
		break;
	case XCB_ENTER_NOTIFY:
	case XCB_FOCUS_IN:
		handle_focus(true);
		break;
	case XCB_LEAVE_NOTIFY:
	case XCB_FOCUS_OUT:
		handle_focus(false);
		break;
	case XCB_SELECTION_CLEAR:
		jbxvt_clear_selection();
		break;
	case XCB_SELECTION_NOTIFY:
		jbxvt_paste_primary(xe->time, xe->requestor, xe->property);
		break;
	case XCB_SELECTION_REQUEST:
		jbxvt_send_selection(xe->time, xe->requestor, xe->target,
			xe->property);
		break;
	case XCB_BUTTON_PRESS:
		handle_button_press(xe);
		break;
	case XCB_BUTTON_RELEASE:
		handle_button_release(xe);
		break;
	case XCB_MOTION_NOTIFY:
		handle_motion_notify(xe);
		break;
	default:
		LOG("Unhandled event %d", xe->type);
	}
	// Zero out event structure for next event:
	jbxvt.com.xev = (struct JBXVTEvent){};
	return true;
}
