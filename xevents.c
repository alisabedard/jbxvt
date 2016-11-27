/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "xevents.h"
#include "cmdtok.h"
#include "command.h"
#include "font.h"
#include "libjb/log.h"
#include "libjb/xcb.h"
#include "lookup_key.h"
#include "mode.h"
#include "mouse.h"
#include "sbar.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "screen.h"
#include "selection.h"
#include "selex.h"
#include "selreq.h"
#include "window.h"
#include <unistd.h>
static void handle_client_message(xcb_connection_t * xc,
	xcb_client_message_event_t * e)
{
	if (e->format == 32 && e->data.data32[0]
		== (unsigned long)jbxvt_get_wm_del_win(xc))
		  exit(0);
}
static void handle_expose(xcb_connection_t * xc,
	xcb_expose_event_t * e)
{
	if (e->window == jbxvt_get_scrollbar(xc))
		jbxvt_draw_scrollbar(xc);
	else
		jbxvt_reset(xc);
}
static void key_press(xcb_connection_t * xc, void * e)
{
	int_fast16_t count = 0;
	uint8_t * s = jbxvt_lookup_key(xc, e, &count);
	if (s)
		jb_require(write(jbxvt_get_fd(), s, count) != -1,
			"Could not write to command");
}
xcb_atom_t jbxvt_get_wm_del_win(xcb_connection_t * xc)
{
	static long unsigned int a;
	if(!a) { // Init on first call:
		a = jb_get_atom(xc, "WM_DELETE_WINDOW");
		xcb_change_property(xc, XCB_PROP_MODE_REPLACE,
			jbxvt_get_main_window(xc),
			jb_get_atom(xc, "WM_PROTOCOLS"),
			XCB_ATOM_ATOM, 32, 1, &a);
	}
	return a;
}
static void handle_motion_notify(xcb_connection_t * xc,
	xcb_motion_notify_event_t * e)
{
	const xcb_window_t w = e->event;
	const struct JBDim b = {.x = e->event_x, .y = e->event_y};
	if (w == jbxvt_get_scrollbar(xc)
		&& (e->state & XCB_KEY_BUT_MASK_BUTTON_2))
		jbxvt_scroll_to(xc, b.y);
	else if (jbxvt_get_mouse_motion_tracked())
		jbxvt_track_mouse(e->detail, e->state, b, JBXVT_MOTION);
	else if ((e->state & XCB_KEY_BUT_MASK_BUTTON_1)
		&& !(e->state & XCB_KEY_BUT_MASK_CONTROL)
		&& !jbxvt_get_mouse_tracked())
		jbxvt_extend_selection(xc, b, true);
}
static void sbop(xcb_connection_t * xc, const int16_t y, const bool up)
{
	if (jbxvt_get_mouse_tracked()) // let the application handle scrolling
		return;
	// xterm's behavior if alternate screen in use is to move the cursor
	if (jbxvt_get_current_screen()
		== jbxvt_get_screen_at(0)) // first screen
		jbxvt_set_scroll(xc, jbxvt_get_scroll()
			+ (up ? -y : y) / jbxvt_get_font_size().h);
	else
		jbxvt_move(xc, 0, up ? -1 : 1, JBXVT_ROW_RELATIVE
			| JBXVT_COLUMN_RELATIVE);
}
static void handle_button_release(xcb_connection_t * xc,
	xcb_button_release_event_t * e)
{
	const xcb_window_t w = e->event;
	const struct JBDim b = {.x = e->event_x, .y = e->event_y};
	const xcb_button_t d = e->detail;
	if (w == jbxvt_get_scrollbar(xc))
		switch (d) {
		case 1:
		case 5:
			sbop(xc, b.y, true);
			break;
		case 3:
		case 4:
			sbop(xc, b.y, false);
			break;
		}
	else if (jbxvt_get_mouse_tracked() && d <= 3)
		/* check less than or equal to 3, since xterm does not
		   report mouse wheel release events.  */
		jbxvt_track_mouse(d, e->state, b, JBXVT_RELEASE);
	else if (!(e->state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (d) {
		case 1:
		case 3:
			jbxvt_make_selection(xc);
			break;
		case 2:
			jbxvt_request_selection(xc, e->time);
			break;
		case 4:
			sbop(xc, b.y, false);
			break;
		case 5:
			sbop(xc, b.y, true);
			break;
		}
	}
}
static void handle_button1_press(xcb_connection_t * xc,
	xcb_button_press_event_t * e, const struct JBDim b)
{
	static unsigned int time1, time2;
	if (e->time - time2 < MP_INTERVAL) {
		time1 = 0;
		time2 = 0;
		jbxvt_start_selection(xc, b, JBXVT_SEL_UNIT_LINE);
	} else if (e->time - time1 < MP_INTERVAL) {
		time2 = e->time;
		jbxvt_start_selection(xc, b, JBXVT_SEL_UNIT_WORD);
	} else {
		time1 = e->time;
		jbxvt_start_selection(xc, b, JBXVT_SEL_UNIT_CHAR);
	}
}
static void handle_button_press(xcb_connection_t * xc,
	xcb_button_press_event_t * e)
{
	const xcb_window_t w = e->event;
	const xcb_window_t v = jbxvt_get_vt_window(xc);
	if (w == v && e->state == XCB_KEY_BUT_MASK_CONTROL) {
		jbxvt_toggle_scrollbar(xc);
		return;
	}
	const struct JBDim b = {.x = e->event_x, .y = e->event_y};
	if (w == v && jbxvt_get_mouse_tracked())
		jbxvt_track_mouse(e->detail, e->state, b, 0);
	else if (w == v && !(e->state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (e->detail) {
		case 1:
			handle_button1_press(xc, e, b);
			break;
		case 3:
			jbxvt_extend_selection(xc, b, false);
			break;
		}
	} else if (w == jbxvt_get_scrollbar(xc) && e->detail == 2)
		jbxvt_scroll_to(xc, b.y);
}
static void handle_focus(const bool in)
{
	if (jbxvt_get_modes()->mouse_focus_evt)
		dprintf(jbxvt_get_fd(), "%s%c]",
			jbxvt_get_csi(), in ? 'I' : 'O');
}
static void handle_selection_notify(xcb_connection_t * xc,
	xcb_selection_notify_event_t * e)
{
	jbxvt_paste_primary(xc, e->time, e->requestor, e->property);
}
static void handle_selection_request(xcb_connection_t * xc,
	xcb_selection_request_event_t * e)
{
	jbxvt_send_selection(xc, e->time, e->requestor, e->target,
		e->property);
}
// Handle X event on queue.  Return true if event was handled.
bool jbxvt_handle_xevents(xcb_connection_t * xc)
{
	jb_check_x(xc);
	xcb_generic_event_t * event = xcb_poll_for_event(xc);
	if (!event) // nothing to process
		return false;
	bool ret = true;
	switch (event->response_type & ~0x80) {
	// Put things to ignore here:
	case 0: // Unimplemented, undefined, no event
	case 150: // Undefined
	case XCB_KEY_RELEASE: // Unimplemented
	case XCB_NO_EXPOSURE: // Unimplemented
	case XCB_REPARENT_NOTIFY: // handle here to ensure cursor filled.
	case XCB_MAP_NOTIFY: // handle here to ensure cursor filled.
		ret = false;
		break;
	case XCB_CONFIGURE_NOTIFY:
		jbxvt_resize_window(xc);
		break;
	case XCB_KEY_PRESS:
		key_press(xc, event);
		break;
	case XCB_CLIENT_MESSAGE:
		handle_client_message(xc,
			(xcb_client_message_event_t *)event);
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		handle_expose(xc, (xcb_expose_event_t *)event);
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
		handle_selection_notify(xc,
			(xcb_selection_notify_event_t *)event);
		break;
	case XCB_SELECTION_REQUEST:
		handle_selection_request(xc,
			(xcb_selection_request_event_t *)event);
		break;
	case XCB_BUTTON_PRESS:
		handle_button_press(xc,
			(xcb_button_press_event_t *)event);
		break;
	case XCB_BUTTON_RELEASE:
		handle_button_release(xc,
			(xcb_button_release_event_t *)event);
		break;
	case XCB_MOTION_NOTIFY:
		handle_motion_notify(xc,
			(xcb_motion_notify_event_t *)event);
		break;
	default:
		LOG("Unhandled event %d", event->response_type);
	}
	free(event);
	return ret;
}
