/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xevents.h"

#include "change_offset.h"
#include "jbxvt.h"
#include "log.h"
#include "Token.h"
#include "xeventst.h"

#include <gc.h>
#include <stdio.h>
#include <stdlib.h>

// Implementation of mouse tracking follows:
enum TrackFlags {
	TRACK_MOTION = 1 << 5,
	TRACK_RELEASE = 1 << 6
};

static void encode_point(xcb_point_t * p)
{
	// encode in X10 format
	p->x += 32;
	p->y += 32;
}

static void track_mouse(uint8_t b, uint32_t state, xcb_point_t p)
{
	LOG("track_mouse(b=%d, p={%d, %d})", b, p.x, p.y);
	// get character position:
	const Size f = jbxvt.X.font_size;
	p.x /= f.w;
	p.y /= f.h;
	// modify for a 1-based row/column system
	++p.x; ++p.y;

	// DECLRP
	cprintf("\033[%d;%d;%d;%d;0&w", b * 2, 7, p.y, p.x);
	//LOG("CSI %d;%d;%d;%d;0&w", b * 2, 0, p.y, p.x);
	// Release handling:
	if (b & TRACK_RELEASE) {
		b = 4; // -1 later on
	} else if (b == 4 || b == 5) {
		// up and down are represented as button one and two,
		// then add 64, plus one since one is lost later
		b += 61; // Wheel mouse handling
	}
	// 4=Shift, 8=Meta, 16=Control
	if (state & XCB_KEY_BUT_MASK_SHIFT) {
		LOG("SHIFT");
		b += 4;
	}
	if (state & XCB_KEY_BUT_MASK_MOD_1) {
		LOG("MOD");
		b += 8;
	}
	if (state & XCB_KEY_BUT_MASK_CONTROL) {
		LOG("CTRL");
		b += 16;
	}
	// encode in X10 format, plus
	// - 1 since 0 is mb 1, and 3 is release
	b += 31;
	encode_point(&p);
	cprintf("\033[M%c%c%c]", b, p.x, p.y);
	LOG("track_mouse: CSI M%cC%cC%c", b, p.x, p.y);
}

static void handle_motion_notify(Token * restrict tk,
	struct xeventst * restrict xe)
{
	if (xe->window == jbxvt.X.win.sb
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_2)) {
		tk->type = TK_SBGOTO;
		tk->arg[0] = xe->box.y;
		tk->nargs = 1;
	} else if (xe->window == jbxvt.X.win.vt
		&& jbxvt.scr.current->ptr_cell) {
		track_mouse(xe->button | TRACK_MOTION, xe->state,
			(xcb_point_t){ xe->box.x, xe->box.y});
	} else if (xe->window == jbxvt.X.win.vt
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_1)
		&& !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		tk->type = TK_SELDRAG;
		tk->arg[0] = xe->box.x;
		tk->arg[1] = xe->box.y;
		tk->nargs = 2;
	}
}

static void sbop(Token * restrict tk, struct xeventst * restrict xe,
	const bool up)
{
	tk->type = up ? TK_SBUP : TK_SBDOWN;
	tk->arg[0] = xe->box.y;
	tk->nargs = 1;
}

static void handle_button_release(Token * restrict tk,
	struct xeventst * restrict xe)
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
		&& jbxvt.scr.current->ptr_xy && xe->button <= 3) {
		/* check less than or equal to 3, since xterm does not
		   report mouse wheel release events.  */
		track_mouse(TRACK_RELEASE, xe->state,
			(xcb_point_t){xe->box.x, xe->box.y});
	} else if (xe->window == jbxvt.X.win.vt
		&& !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (xe->button) {
		case 1:
		case 3:
			tk->type = TK_SELECT;
			tk->arg[0] = xe->time;
			tk->nargs = 1;
			break;
		case 2:
			tk->type = TK_SELINSRT;
			tk->arg[0] = xe->time;
			tk->arg[1] = xe->box.x;
			tk->arg[2] = xe->box.y;
			tk->nargs = 3;
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

static void handle_button1_press(Token * restrict tk,
	struct xeventst * restrict xe)
{
	static unsigned int time1, time2;
	if (xe->time - time2
		< MP_INTERVAL) {
		time1 = 0;
		time2 = 0;
		tk->type = TK_SELLINE;
	} else if (xe->time - time1
		< MP_INTERVAL) {
		time2 = xe->time;
		tk->type = TK_SELWORD;
	} else {
		time1 = xe->time;
		tk->type = TK_SELSTART;
	}
}

static void handle_button_press(Token * restrict tk,
	struct xeventst * restrict xe)
{
	if (xe->window == jbxvt.X.win.vt
		&& xe->state == XCB_KEY_BUT_MASK_CONTROL) {
		tk->type = TK_SBSWITCH;
		tk->nargs = 0;
		return;
	}
	if (xe->window == jbxvt.X.win.vt && jbxvt.scr.current->ptr_xy)
		track_mouse(xe->button, xe->state,
			(xcb_point_t){xe->box.x, xe->box.y});
	else if (xe->window == jbxvt.X.win.vt
		&& (xe->state & XCB_KEY_BUT_MASK_CONTROL) == 0) {
		switch (xe->button) {
		case 1:
			handle_button1_press(tk, xe);
			break;
		case 2:
			tk->type = TK_NULL;
			break;
		case 3:
			tk->type = TK_SELEXTND;
			break;
		}
		tk->arg[0] = xe->box.x;
		tk->arg[1] = xe->box.y;
		tk->nargs = 2;
		return;
	}
	if (xe->window == jbxvt.X.win.sb) {
		if (xe->button == 2) {
			tk->type = TK_SBGOTO;
			tk->arg[0] = xe->box.y;
			tk->nargs = 1;
		}
	}
}

// convert next X event into a token
bool handle_xevents(Token * restrict tk)
{
	struct xeventst *xe = pop_xevent();
	if(!xe) return false;
	if (xe->window == jbxvt.X.win.vt)
		  tk->region = REGION_SCREEN;
	else if (xe->window == jbxvt.X.win.sb)
		  tk->region = REGION_SCROLLBAR;
	else if (xe->window == jbxvt.X.win.main)
		  tk->region = REGION_MAINWIN;
	else
		  tk->region = REGION_NONE;
	switch (xe->type) {
	case XCB_KEY_RELEASE: // Unimplemented
		break;
	case XCB_ENTER_NOTIFY:
		tk->type = TK_ENTRY;
		tk->arg[0] = 1;
		tk->nargs = 1;
		break;
	case XCB_LEAVE_NOTIFY:
		tk->type = TK_ENTRY;
		tk->arg[0] = 0;
		tk->nargs = 1;
		break;
	case XCB_FOCUS_IN:
		tk->type = TK_FOCUS;
		tk->arg[0] = 1;
		tk->arg[1] = xe->detail;
		tk->nargs = 2;
		break;
	case XCB_FOCUS_OUT:
		tk->type = TK_FOCUS;
		tk->arg[0] = 0;
		tk->arg[1] = xe->detail;
		tk->nargs = 2;
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		tk->type = TK_EXPOSE;
		tk->arg[0] = xe->box.x;
		tk->arg[1] = xe->box.y;
		tk->arg[2] = xe->box.width;
		tk->arg[3] = xe->box.height;
		tk->nargs = 4;
		break;
	case XCB_NO_EXPOSURE: // Unimplemented
		break;
	case XCB_CONFIGURE_NOTIFY:
		LOG("ConfigureNotify");
		tk->type = TK_RESIZE;
		tk->nargs = 0;
		break;
	case XCB_SELECTION_CLEAR:
		tk->type = TK_SELCLEAR;
		tk->arg[0] = xe->time;
		tk->nargs = 1;
		break;
	case XCB_SELECTION_NOTIFY:
		tk->type = TK_SELNOTIFY;
		tk->arg[0] = xe->time;
		tk->arg[1] = xe->requestor;
		tk->arg[2] = xe->property;
		tk->nargs = 3;
		break;
	case XCB_SELECTION_REQUEST:
		tk->type = TK_SELREQUEST;
		tk->arg[0] = xe->time;
		tk->arg[1] = xe->requestor;
		tk->arg[2] = xe->target;
		tk->arg[3] = xe->property;
		tk->nargs = 4;
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
		LOG("Unhandled event %d", xe->type);
	}
	return true;
}

