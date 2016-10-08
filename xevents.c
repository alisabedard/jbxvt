/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xevents.h"

#include "jbxvt.h"
#include "libjb/log.h"
#include "mouse.h"
#include "sbar.h"
#include "screen.h"
#include "selex.h"
#include "selreq.h"
#include "Token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

xcb_atom_t wm_del_win(void)
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

static void set_args(const TokenType type, struct Token * restrict tk,
	int32_t * restrict args, uint8_t nargs)
{
	nargs = MIN(nargs, TK_MAX_ARGS); // prevent buffer overflow
	memcpy(tk->arg, args, nargs * sizeof(int32_t));
	tk->nargs = nargs;
	tk->type = type;
}

#define ARGS(type, ...) set_args(type, tk, (int32_t[]){__VA_ARGS__},\
	sizeof((int32_t[]){__VA_ARGS__}))

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

static void sbop(struct Token * restrict tk, struct JBXVTEvent * restrict xe,
	const bool up)
{
	if (jbxvt_get_mouse_tracked()) // let the application handle scrolling
		return;
	// xterm's behavior if alternate screen in use is to move the cursor
	jbxvt.scr.current == &jbxvt.scr.s[0] ?  ARGS(up ? TK_SBUP : TK_SBDOWN,
		xe->box.y) : ARGS(up ? TK_CUU : TK_CUD, 1);
}

static void handle_button_release(struct Token * restrict tk,
	struct JBXVTEvent * restrict xe)
{
	if (xe->window == jbxvt.X.win.sb) {
		switch (xe->button) {
		case 1:
		case 5:
			sbop(tk, xe, true);
			break;
		case 3:
		case 4:
			sbop(tk, xe, false);
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
			ARGS(TK_SELINSRT, xe->time, xe->box.x, xe->box.y);
			break;
		case 4:
			sbop(tk, xe, false);
			break;
		case 5:
			sbop(tk, xe, true);
			break;

		}
	}
}

static TokenType handle_button1_press(struct JBXVTEvent * restrict xe)
{
	static unsigned int time1, time2;
	if (xe->time - time2
		< MP_INTERVAL) {
		time1 = 0;
		time2 = 0;
		return TK_SELLINE;
	} else if (xe->time - time1
		< MP_INTERVAL) {
		time2 = xe->time;
		return TK_SELWORD;
	}
	time1 = xe->time;
	return TK_SELSTART;
}

static void handle_button_press(struct Token * restrict tk,
	struct JBXVTEvent * restrict xe)
{
	const xcb_window_t v = jbxvt.X.win.vt;
	if (xe->window == v && xe->state == XCB_KEY_BUT_MASK_CONTROL) {
		ARGS(TK_SBSWITCH);
		return;
	}
	if (xe->window == v && jbxvt_get_mouse_tracked())
		jbxvt_track_mouse(xe->button, xe->state,
			(struct JBDim){.x = xe->box.x, .y = xe->box.y}, 0);
	else if (xe->window == v && !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		TokenType type = TK_NULL;
		switch (xe->button) {
		case 1:
			type = handle_button1_press(xe);
			break;
		case 3:
			type = TK_SELEXTND;
			break;
		}
		ARGS(type, xe->box.x, xe->box.y);
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

// convert next X event into a token
bool handle_xevents(struct Token * restrict tk)
{
	struct JBXVTEvent * xe = &jbxvt.com.xev;
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
		handle_button_press(tk, xe);
		break;
	case XCB_BUTTON_RELEASE:
		handle_button_release(tk, xe);
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

