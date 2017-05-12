/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "button_events.h"
#include "JBXVTSelectionUnit.h"
#include "command.h"
#include "font.h"
#include "libjb/JBDim.h"
#include "mouse.h"
#include "move.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "selex.h"
#include "selreq.h"
#include "window.h"
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
void jbxvt_handle_button_release_event(xcb_connection_t * xc,
	xcb_button_release_event_t * e)
{
	enum { SCROLL_INCREMENT = 64 };
	const xcb_window_t window = e->event;
	const xcb_button_t button = e->detail;
	if (window == jbxvt_get_scrollbar(xc))
		switch (button) {
		case 1:
			sbop(xc, SCROLL_INCREMENT << 1, true);
			break;
		case 3:
			sbop(xc, SCROLL_INCREMENT << 1, false);
			break;
		case 4:
			sbop(xc, SCROLL_INCREMENT, false);
			break;
		case 5:
			sbop(xc, SCROLL_INCREMENT, true);
			break;
		}
	else if (jbxvt_get_mouse_tracked() && button <= 3)
		/* check less than or equal to 3, since xterm does not
		   report mouse wheel release events.  */
		jbxvt_track_mouse(button, e->state, (struct JBDim){
			.x = e->event_x, .y = e->event_y}, JBXVT_RELEASE);
	else if (!(e->state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (button) {
		case 1:
		case 3:
			jbxvt_make_selection(xc);
			break;
		case 2:
			jbxvt_request_selection(xc, e->time);
			break;
		case 4:
			sbop(xc, SCROLL_INCREMENT, false);
			break;
		case 5:
			sbop(xc, SCROLL_INCREMENT, true);
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
void jbxvt_handle_button_press_event(xcb_connection_t * xc,
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

