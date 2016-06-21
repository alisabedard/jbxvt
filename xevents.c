/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xevents.h"

#include "change_offset.h"
#include "jbxvt.h"
#include "log.h"
#include "token.h"
#include "xeventst.h"

#include <gc.h>
#include <stdio.h>
#include <stdlib.h>

// Implementation of mouse tracking follows:
enum TrackFlags {
	TRACK_RELEASE = 1 << 5,
	TRACK_MOTION = 1 << 4
};
static void track_mouse(uint8_t b, uint32_t state, xcb_point_t p)
{
	LOG("track_mouse(b=%d, p={%d, %d})", b, p.x, p.y);
	// get character position:
	p.x /= jbxvt.X.font_width;
	p.y /= jbxvt.X.font_height;
	// modify for a 1-based row/column system
	p.x += 1;
	p.y += 1;

	// DECLRP
	//cprintf("\033[%d;%d;%d;%d;0&w", b * 2, 7, p.y, p.x);
	//LOG("CSI %d;%d;%d;%d;0&w", b * 2, 0, p.y, p.x);

	// Release handling:
	if (b & TRACK_RELEASE) {
		b = 4; // -1 later on
	} else if (b == 4 || b == 5) {
		// Wheel mouse handling:
		b += 64;
	}

	// 4=Shift, 8=Meta, 16=Control
	if (state & XCB_KEY_BUT_MASK_SHIFT)
		  b |= 4;
	if (state & (XCB_KEY_BUT_MASK_MOD_1 | XCB_KEY_BUT_MASK_MOD_2
		| XCB_KEY_BUT_MASK_MOD_3 | XCB_KEY_BUT_MASK_MOD_4))
		  b |= 8;
	if (state & XCB_KEY_BUT_MASK_CONTROL)
		  b |= 16;

	// encode in X10 format
	b += 31; // - 1 since 0 is mb 1
	p.x += 32;
	p.y += 32;

	cprintf("\033[M%c%c%c]", b, p.x, p.y);
	LOG("track_mouse: CSI M%cC%cC%c", b, p.x, p.y);
}

static void handle_motion_notify(struct tokenst * restrict tk,
	struct xeventst * restrict xe)
{
	if (xe->xe_window == jbxvt.X.win.sb
		&& (xe->xe_state & XCB_KEY_BUT_MASK_BUTTON_2)) {
			tk->tk_type = TK_SBGOTO;
			tk->tk_arg[0] = xe->xe_y;
			tk->tk_nargs = 1;
	} else if (xe->xe_window == jbxvt.X.win.vt
		&& jbxvt.scr.current->ptr_cell) {
		track_mouse(xe->xe_button | TRACK_MOTION, xe->xe_state,
			(xcb_point_t){ xe->xe_x, xe->xe_y});
	} else if (xe->xe_window == jbxvt.X.win.vt
		&& (xe->xe_state & XCB_KEY_BUT_MASK_BUTTON_1)
		&& !(xe->xe_state & XCB_KEY_BUT_MASK_CONTROL)) {
		tk->tk_type = TK_SELDRAG;
		tk->tk_arg[0] = xe->xe_x;
		tk->tk_arg[1] = xe->xe_y;
		tk->tk_nargs = 2;
	}
}

static void sbop(struct tokenst * restrict tk, struct xeventst * restrict xe,
	const bool up)
{
	tk->tk_type = up ? TK_SBUP : TK_SBDOWN;
	tk->tk_arg[0] = xe->xe_y;
	tk->tk_nargs = 1;
}

static void handle_button_release(struct tokenst * restrict tk,
	struct xeventst * restrict xe)
{
	LOG("handle_button_release()");
	if (xe->xe_window == jbxvt.X.win.sb) {
		switch (xe->xe_button) {
		case 1:
		case 5:
			sbop(tk, xe, true);
			break;
		case 3:
		case 4:
			sbop(tk, xe, false);
			break;
		}
	} else if (xe->xe_window == jbxvt.X.win.vt && jbxvt.scr.current->ptr_xy) {
		if (xe->xe_button > 3)
			  return; // xterm doesn't report mouse wheel release
		track_mouse(xe->xe_button | TRACK_RELEASE,
			xe->xe_state, (xcb_point_t){xe->xe_x, xe->xe_y});
	} else if (xe->xe_window == jbxvt.X.win.vt
		&& !(xe->xe_state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (xe->xe_button) {
		case 1:
		case 3:
			tk->tk_type = TK_SELECT;
			tk->tk_arg[0] = xe->xe_time;
			tk->tk_nargs = 1;
			break;
		case 2:
			tk->tk_type = TK_SELINSRT;
			tk->tk_arg[0] = xe->xe_time;
			tk->tk_arg[1] = xe->xe_x;
			tk->tk_arg[2] = xe->xe_y;
			tk->tk_nargs = 3;
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

static void handle_button1_press(struct tokenst * restrict tk,
	struct xeventst * restrict xe)
{
	LOG("handle_button1_press");
	static unsigned int time1, time2;
	if (xe->xe_time - time2
		< MP_INTERVAL) {
		time1 = 0;
		time2 = 0;
		tk->tk_type = TK_SELLINE;
	} else if (xe->xe_time - time1
		< MP_INTERVAL) {
		time2 = xe->xe_time;
		tk->tk_type = TK_SELWORD;
	} else {
		time1 = xe->xe_time;
		tk->tk_type = TK_SELSTART;
	}
}

static void handle_button_press(struct tokenst * restrict tk,
	struct xeventst * restrict xe)
{
	LOG("handle_button_press()");
	if (xe->xe_window == jbxvt.X.win.vt
		&& xe->xe_state == XCB_KEY_BUT_MASK_CONTROL) {
		tk->tk_type = TK_SBSWITCH;
		tk->tk_nargs = 0;
		return;
	}
	if (xe->xe_window == jbxvt.X.win.vt && jbxvt.scr.current->ptr_xy)
		track_mouse(xe->xe_button, xe->xe_state,
			(xcb_point_t){xe->xe_x, xe->xe_y});
	else if (xe->xe_window == jbxvt.X.win.vt
		&& (xe->xe_state & XCB_KEY_BUT_MASK_CONTROL) == 0) {
		switch (xe->xe_button) {
		case 1:
			handle_button1_press(tk, xe);
			break;
		case 2:
			tk->tk_type = TK_NULL;
			break;
		case 3:
			tk->tk_type = TK_SELEXTND;
			break;
		}
		tk->tk_arg[0] = xe->xe_x;
		tk->tk_arg[1] = xe->xe_y;
		tk->tk_nargs = 2;
		return;
	}
	if (xe->xe_window == jbxvt.X.win.sb) {
		if (xe->xe_button == 2) {
			tk->tk_type = TK_SBGOTO;
			tk->tk_arg[0] = xe->xe_y;
			tk->tk_nargs = 1;
		}
	}
}

// convert next X event into a token
bool handle_xevents(struct tokenst * restrict tk)
{
	struct xeventst *xe = pop_xevent();
	if(!xe) return false;
	if (xe->xe_window == jbxvt.X.win.vt)
		  tk->tk_region = SCREEN;
	else if (xe->xe_window == jbxvt.X.win.sb)
		  tk->tk_region = SCROLLBAR;
	else if (xe->xe_window == jbxvt.X.win.main)
		  tk->tk_region = MAINWIN;
	else
		  tk->tk_region = -1;
	switch (xe->xe_type) {
	case XCB_KEY_RELEASE: // Unimplemented
		break;
	case XCB_ENTER_NOTIFY:
		tk->tk_type = TK_ENTRY;
		tk->tk_arg[0] = 1;
		tk->tk_nargs = 1;
		break;
	case XCB_LEAVE_NOTIFY:
		tk->tk_type = TK_ENTRY;
		tk->tk_arg[0] = 0;
		tk->tk_nargs = 1;
		break;
	case XCB_FOCUS_IN:
		tk->tk_type = TK_FOCUS;
		tk->tk_arg[0] = 1;
		tk->tk_arg[1] = xe->xe_detail;
		tk->tk_nargs = 2;
		break;
	case XCB_FOCUS_OUT:
		tk->tk_type = TK_FOCUS;
		tk->tk_arg[0] = 0;
		tk->tk_arg[1] = xe->xe_detail;
		tk->tk_nargs = 2;
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		tk->tk_type = TK_EXPOSE;
		tk->tk_arg[0] = xe->xe_x;
		tk->tk_arg[1] = xe->xe_y;
		tk->tk_arg[2] = xe->xe_width;
		tk->tk_arg[3] = xe->xe_height;
		tk->tk_nargs = 4;
		break;
	case XCB_NO_EXPOSURE: // Unimplemented
		break;
	case XCB_CONFIGURE_NOTIFY:
		LOG("ConfigureNotify");
		tk->tk_type = TK_RESIZE;
		tk->tk_nargs = 0;
		break;
	case XCB_SELECTION_CLEAR:
		tk->tk_type = TK_SELCLEAR;
		tk->tk_arg[0] = xe->xe_time;
		tk->tk_nargs = 1;
		break;
	case XCB_SELECTION_NOTIFY:
		tk->tk_type = TK_SELNOTIFY;
		tk->tk_arg[0] = xe->xe_time;
		tk->tk_arg[1] = xe->xe_requestor;
		tk->tk_arg[2] = xe->xe_property;
		tk->tk_nargs = 3;
		break;
	case XCB_SELECTION_REQUEST:
		tk->tk_type = TK_SELREQUEST;
		tk->tk_arg[0] = xe->xe_time;
		tk->tk_arg[1] = xe->xe_requestor;
		tk->tk_arg[2] = xe->xe_target;
		tk->tk_arg[3] = xe->xe_property;
		tk->tk_nargs = 4;
		break;
	case XCB_BUTTON_PRESS:
		handle_button_press(tk, xe);
		break;
	case XCB_BUTTON_RELEASE:
		handle_button_release(tk, xe);
		break;
	case XCB_MOTION_NOTIFY:
		handle_motion_notify(tk, xe);
		break;
	default:
		LOG("Unhandled event %d", xe->xe_type);
	}
	return true;
}

