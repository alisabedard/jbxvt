/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xevents.h"

#include "jbxvt.h"
#include "libjb/log.h"
#include "Token.h"
#include "xeventst.h"

#include <gc.h>
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

// Implementation of mouse tracking follows:
enum TrackFlags {
	TRACK_MOTION = 0x20,
	TRACK_RELEASE = 0x40
};

static uint8_t get_mod(const uint16_t state)
{
	// 4=Shift, 8=Meta, 16=Control
	uint8_t mod = 0;
	if (state & XCB_KEY_BUT_MASK_SHIFT)
		mod += 4;
	if (state & XCB_KEY_BUT_MASK_MOD_1)
		mod += 8;
	if (state & XCB_KEY_BUT_MASK_CONTROL)
		mod += 16;
	return mod;
}

static bool track_mouse_sgr(uint8_t b, xcb_point_t p)
{
	if (!jbxvt.mode.mouse_sgr)
		return false;
	const bool rel = b & TRACK_RELEASE;
	if (rel)
		b &= ~TRACK_RELEASE;
	cprintf("\033[<%c;%c;%c%c", b, p.x, p.y, rel ? 'm' : 'M');
	return true;
}

static void to_pixels(xcb_point_t * p)
{
	const Size f = jbxvt.X.f.size;
	p->x *= f.w;
	p->y *= f.h;
}

static void to_chars(xcb_point_t * p)
{
	const Size f = jbxvt.X.f.size;
	p->x /= f.w;
	p->y /= f.h;
}

static void locator_report(const uint8_t b, xcb_point_t p)
{
	if (jbxvt.opt.elr) {
		uint8_t a[2];
		a[0] = jbxvt.opt.elr & 0x3;
		a[1] = (jbxvt.opt.elr & 0xc) >> 2;
		if (!a[0]) // disabled
			return;
		if (a[0] == 2) // only enabled for one report
			jbxvt.opt.elr = 0;
		if (a[1] == 1) // report in pixels
			to_pixels(&p);
		// DECLRP
		cprintf("\033[%d;%d;%d;%d;0&w", b * 2, 7, p.y, p.x);
	}
}

static void track_mouse(uint8_t b, uint32_t state, xcb_point_t p)
{
	LOG("track_mouse(b=%d, p={%d, %d})", b, p.x, p.y);
	// get character position:
	to_chars(&p);
	// modify for a 1-based row/column system
	++p.x; ++p.y;
	const bool wheel = b == 4 || b == 5;
	locator_report(b, p);
	struct JBXVTPrivateModes * m = &jbxvt.mode;
	// Release handling:
	if (b & TRACK_RELEASE) {
		if (m->mouse_x10)
			return; // release untracked in x10 mode
		LOG("TRACK_RELEASE");
		if (!m->mouse_sgr) // sgr reports which button was released
			b = 3; // release code
	} else if (wheel) { // wheel release untracked
		// up and down are represented as button one(0) and two(1),
		// then add 64, plus one since one was lost earlier
		b += 61; // Wheel mouse handling
		LOG("wheel b: %d", b);
	}
	// base button on 0:
	--b;
	// add modifiers:
	b += get_mod(state);
	if (track_mouse_sgr(b, p)) {
		LOG("mouse_sgr");
		return;
	}
	// X10 encoding:
	b += 32;
	p.x += 32;
	p.y += 32;
	char * format = m->mouse_urxvt ? "\033[%d;%d;%dM" : "\033[M%c%c%c";
#ifndef DEBUG
	cprintf(format, b, p.x, p.y);
#else//DEBUG
	char * out = strdup(cprintf(format, b, p.x, p.y));
	for(char * i = out; *i; ++i)
		if (*i == '\033')
			*i = 'E';
	fprintf(stderr, "track_mouse: %s\n", out);
	free(out);
#endif//!DEBUG
}

static bool is_motion_tracked(void)
{
	struct JBXVTPrivateModes * m = &jbxvt.mode;
	bool r = false;
	r |= m->mouse_btn_evt;
	r |= m->mouse_any_evt;
	return r;
}

static bool is_tracked(void)
{
	struct JBXVTPrivateModes * m = &jbxvt.mode;
	bool r = false;
	r |= m->mouse_x10;
	r |= m->mouse_vt200;
	r |= m->mouse_vt200hl;
	r |= m->mouse_ext;
	r |= m->mouse_sgr;
	r |= m->mouse_urxvt;
	r |= is_motion_tracked();
	return r;
}

static void handle_motion_notify(Token * restrict tk,
	struct xeventst * restrict xe)
{
	if (xe->window == jbxvt.X.win.sb
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_2)) {
		tk->type = TK_SBGOTO;
		tk->arg[0] = xe->box.y;
		tk->nargs = 1;
	} else if (xe->window == jbxvt.X.win.vt && is_motion_tracked()) {
		track_mouse(xe->button | TRACK_MOTION, xe->state,
			(xcb_point_t){ xe->box.x, xe->box.y});
	} else if (xe->window == jbxvt.X.win.vt
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_1)
		&& !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		if (is_tracked())
			  return;
		tk->type = TK_SELDRAG;
		tk->arg[0] = xe->box.x;
		tk->arg[1] = xe->box.y;
		tk->nargs = 2;
	}
}

static void sbop(Token * restrict tk, struct xeventst * restrict xe,
	const bool up)
{
	if (is_tracked()) // let the application handle scrolling
		return;
	if (jbxvt.scr.current == &jbxvt.scr.s[0]) {
		tk->type = up ? TK_SBUP : TK_SBDOWN;
		tk->arg[0] = xe->box.y;
		tk->nargs = 1;
	} else { // xterm's behavior if alternate screen in use:
		tk->type = up ? TK_CUU : TK_CUD;
		tk->arg[0] = 1;
		tk->nargs = 1;
	}
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
		&& is_tracked() && xe->button <= 3) {
		/* check less than or equal to 3, since xterm does not
		   report mouse wheel release events.  */
		track_mouse(xe->button | TRACK_RELEASE, xe->state,
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
	const xcb_window_t v = jbxvt.X.win.vt;
	if (xe->window == v && xe->state == XCB_KEY_BUT_MASK_CONTROL) {
		tk->type = TK_SBSWITCH;
		tk->nargs = 0;
		return;
	}
	if (xe->window == v && is_tracked())
		track_mouse(xe->button, xe->state,
			(xcb_point_t){xe->box.x, xe->box.y});
	else if (xe->window == v && !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
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
	switch (xe->type) { // Ordered numerically:
	case 0: // Unimplemented, undefined
	case 150: // Undefined
	case XCB_KEY_RELEASE: // Unimplemented
	case XCB_NO_EXPOSURE: // Unimplemented
		break;
	case XCB_REPARENT_NOTIFY: // handle here to ensure cursor filled.
	case XCB_MAP_NOTIFY: // handle here to ensure cursor filled.
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
	case XCB_CONFIGURE_NOTIFY:
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

