/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xevents.h"

#include "jbxvt.h"
#include "libjb/log.h"
#include "screen.h"
#include "Token.h"

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

static uint8_t get_mod(const uint16_t state)
{
	// 4=Shift, 8=Meta, 16=Control
	uint8_t mod = 0;
#define MOD(mk, n) if(state&XCB_KEY_BUT_MASK_##mk) mod+=n;
	MOD(SHIFT, 4); MOD(MOD_1, 8); MOD(CONTROL, 16);
	return mod;
}

enum {
	JBXVT_RELEASE = 1,
	JBXVT_MOTION = 2
};

#define MD jbxvt.mode

static bool track_mouse_sgr(uint8_t b, struct JBDim p, const bool rel)
{
	if (!MD.mouse_sgr)
		return false;
	cprintf("\033[<%c;%c;%c%c", b, p.x, p.y, rel ? 'm' : 'M');
	return true;
}

static void locator_report(const uint8_t b, struct JBDim p)
{
	if (!MD.elr)
		return;
	if (MD.elr_once)
		MD.elr_once = MD.elr = false;
	if (MD.elr_pixels)
		p = get_p(p);
	// DECLRP
	cprintf("\033[%d;%d;%d;%d;0&w", b * 2, 7, p.y, p.x);
}

static void track_mouse(uint8_t b, uint32_t state, struct JBDim p,
	const uint8_t flags)
{
	LOG("track_mouse(b=%d, p={%d, %d})", b, p.x, p.y);
	// get character position:
	p = get_c(p);
	// modify for a 1-based row/column system
	++p.x; ++p.y;
	const bool wheel = b == 4 || b == 5;
	locator_report(b, p);
	struct JBXVTPrivateModes * m = &jbxvt.mode;
	// Release handling:
	if (flags & JBXVT_RELEASE) {
		if (m->mouse_x10 || wheel) // wheel release untracked
			return; // release untracked in x10 mode
		LOG("TRACK_RELEASE");
		if (!m->mouse_sgr) // sgr reports which button was released
			b = 4; // release code, -1 later
	} else if (wheel) { // wheel release untracked
		b += 65; // Wheel mouse handling
		LOG("wheel b: %d", b);
	}
	// base button on 0:
	--b;
	// add modifiers:
	b += get_mod(state);
	if (track_mouse_sgr(b, p, flags & JBXVT_RELEASE)) {
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
#define TRK(it) r|=m->mouse_##it
	TRK(btn_evt); TRK(any_evt);
	return r;
}

static bool is_tracked(void)
{
	struct JBXVTPrivateModes * m = &jbxvt.mode;
	bool r = false;
	TRK(x10); TRK(vt200); TRK(vt200hl); TRK(ext); TRK(sgr); TRK(urxvt);
	r |= is_motion_tracked();
	return r;
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

static void handle_motion_notify(struct Token * restrict tk,
	struct JBXVTEvent * restrict xe)
{
	if (xe->window == jbxvt.X.win.sb
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_2)) {
		ARGS(TK_SBGOTO, xe->box.y);
	} else if (xe->window == jbxvt.X.win.vt && is_motion_tracked()) {
		track_mouse(xe->button, xe->state, (struct JBDim){
			.x = xe->box.x, .y = xe->box.y}, JBXVT_MOTION);
	} else if (xe->window == jbxvt.X.win.vt
		&& (xe->state & XCB_KEY_BUT_MASK_BUTTON_1)
		&& !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		if (is_tracked())
			  return;
		ARGS(TK_SELDRAG, xe->box.x, xe->box.y);
	}
}

static void sbop(struct Token * restrict tk, struct JBXVTEvent * restrict xe,
	const bool up)
{
	if (is_tracked()) // let the application handle scrolling
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
		&& is_tracked() && xe->button <= 3) {
		/* check less than or equal to 3, since xterm does not
		   report mouse wheel release events.  */
		track_mouse(xe->button, xe->state,
			(struct JBDim){.x = xe->box.x, .y = xe->box.y},
			JBXVT_RELEASE);
	} else if (xe->window == jbxvt.X.win.vt
		&& !(xe->state & XCB_KEY_BUT_MASK_CONTROL)) {
		switch (xe->button) {
		case 1:
		case 3:
			ARGS(TK_SELECT, xe->time);
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
	if (xe->window == v && is_tracked())
		track_mouse(xe->button, xe->state,
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
		ARGS(TK_SBGOTO, xe->box.y);
}

static struct JBXVTEvent * pop_xevent(void)
{
	struct JBXVTEventQueue * q = &jbxvt.com.events;
	struct JBXVTEvent * xe = q->last;
	if (xe) {
		q->last = xe->prev;
		*(q->last ? &q->last->next : &q->start) = NULL;
	}
	return xe;
}

static enum Region get_region(struct JBXVTEvent * xe)
{
	const xcb_window_t w = xe->window;
	const struct JBXVTXWindows * j = &jbxvt.X.win;
	if (w == j->vt)
		return REGION_SCREEN;
	else if (w == j->sb)
		return REGION_SCROLLBAR;
	else if (w == j->main)
		return REGION_MAINWIN;
	return REGION_NONE;
}

// convert next X event into a token
bool handle_xevents(struct Token * restrict tk)
{
	struct JBXVTEvent * xe = pop_xevent();
	if(!xe)
		return false;
	tk->region = get_region(xe);
	switch (xe->type &~0x80) { // Ordered numerically:
	case 0: // Unimplemented, undefined
	case 150: // Undefined
	case XCB_KEY_RELEASE: // Unimplemented
	case XCB_NO_EXPOSURE: // Unimplemented
		break;
	case XCB_REPARENT_NOTIFY: // handle here to ensure cursor filled.
	case XCB_MAP_NOTIFY: // handle here to ensure cursor filled.
	case XCB_ENTER_NOTIFY:
		ARGS(TK_ENTRY, 1);
		break;
	case XCB_LEAVE_NOTIFY:
		ARGS(TK_ENTRY, 0);
		break;
	case XCB_FOCUS_IN:
		ARGS(TK_FOCUS, 1, xe->detail);
		break;
	case XCB_FOCUS_OUT:
		ARGS(TK_FOCUS, 0, xe->detail);
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		ARGS(TK_EXPOSE, xe->box.x, xe->box.y,
			xe->box.width, xe->box.height);
		break;
	case XCB_CONFIGURE_NOTIFY:
		ARGS(TK_RESIZE);
		break;
	case XCB_SELECTION_CLEAR:
		ARGS(TK_SELCLEAR, xe->time);
		break;
	case XCB_SELECTION_NOTIFY:
		ARGS(TK_SELNOTIFY, xe->time, xe->requestor, xe->property);
		break;
	case XCB_SELECTION_REQUEST:
		ARGS(TK_SELREQUEST, xe->time, xe->requestor,
			xe->target, xe->property);
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
	free(xe);
	return true;
}

