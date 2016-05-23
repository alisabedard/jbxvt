/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xevents.h"

#include "jbxvt.h"
#include "log.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>

static void handle_motion_notify(struct tokenst * restrict tk,
	struct xeventst * restrict xe)
{
	if (xe->xe_window == jbxvt.X.win.sb && (xe->xe_state & Button2Mask)) {
		Window w;
		int d, y;
		unsigned int mods;
		// Only interested in y and mods
		XQueryPointer(jbxvt.X.dpy, jbxvt.X.win.sb, &w, &w,
			&d, &d, &d, &y, &mods);
		if (mods & Button2Mask) {
			tk->tk_type = TK_SBGOTO;
			tk->tk_arg[0] = y;
			tk->tk_nargs = 1;
		}
		return;
	}
	if (xe->xe_window == jbxvt.X.win.vt
		&& (xe->xe_state & Button1Mask)
		&& !(xe->xe_state & ControlMask)) {
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
	if (xe->xe_window == jbxvt.X.win.sb) {
		switch (xe->xe_button) {
		case Button1:
		case Button5:
			sbop(tk, xe, true);
			break;
		case Button3:
		case Button4:
			sbop(tk, xe, false);
			break;
		}
	} else if (xe->xe_window == jbxvt.X.win.vt
		&& !(xe->xe_state & ControlMask)) {
		switch (xe->xe_button) {
		case Button1:
		case Button3:
			tk->tk_type = TK_SELECT;
			tk->tk_arg[0] = xe->xe_time;
			tk->tk_nargs = 1;
			break;
		case Button2:
			tk->tk_type = TK_SELINSRT;
			tk->tk_arg[0] = xe->xe_time;
			tk->tk_arg[1] = xe->xe_x;
			tk->tk_arg[2] = xe->xe_y;
			tk->tk_nargs = 3;
			break;
		case Button4:
			sbop(tk, xe, false);
			break;
		case Button5:
			sbop(tk, xe, true);
			break;

		}
	}
}

static void handle_button1_press(struct tokenst * restrict tk,
	struct xeventst * restrict xe)
{
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
	if (xe->xe_window == jbxvt.X.win.vt
		&& xe->xe_state == ControlMask) {
		tk->tk_type = TK_SBSWITCH;
		tk->tk_nargs = 0;
		return;
	}
	if (xe->xe_window == jbxvt.X.win.vt
		&& (xe->xe_state & ControlMask) == 0) {
		switch (xe->xe_button) {
		case Button1 :
			handle_button1_press(tk, xe);
			break;
		case Button2 :
			tk->tk_type = TK_NULL;
			break;
		case Button3 :
			tk->tk_type = TK_SELEXTND;
			break;
		}
		tk->tk_arg[0] = xe->xe_x;
		tk->tk_arg[1] = xe->xe_y;
		tk->tk_nargs = 2;
		return;
	}
	if (xe->xe_window == jbxvt.X.win.sb) {
		if (xe->xe_button == Button2) {
			tk->tk_type = TK_SBGOTO;
			tk->tk_arg[0] = xe->xe_y;
			tk->tk_nargs = 1;
		}
	}
}

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
	case EnterNotify :
		tk->tk_type = TK_ENTRY;
		tk->tk_arg[0] = 1;
		tk->tk_nargs = 1;
		break;
	case LeaveNotify :
		tk->tk_type = TK_ENTRY;
		tk->tk_arg[0] = 0;
		tk->tk_nargs = 1;
		break;
	case FocusIn :
		tk->tk_type = TK_FOCUS;
		tk->tk_arg[0] = 1;
		tk->tk_arg[1] = xe->xe_detail;
		tk->tk_nargs = 2;
		break;
	case FocusOut :
		tk->tk_type = TK_FOCUS;
		tk->tk_arg[0] = 0;
		tk->tk_arg[1] = xe->xe_detail;
		tk->tk_nargs = 2;
		break;
	case Expose :
		tk->tk_type = TK_EXPOSE;
		tk->tk_arg[0] = xe->xe_x;
		tk->tk_arg[1] = xe->xe_y;
		tk->tk_arg[2] = xe->xe_width;
		tk->tk_arg[3] = xe->xe_height;
		tk->tk_nargs = 4;
		break;
	case ConfigureNotify :
		LOG("ConfigureNotify");
		tk->tk_type = TK_RESIZE;
		tk->tk_nargs = 0;
		break;
	case SelectionClear :
		tk->tk_type = TK_SELCLEAR;
		tk->tk_arg[0] = xe->xe_time;
		tk->tk_nargs = 1;
		break;
	case SelectionNotify :
		tk->tk_type = TK_SELNOTIFY;
		tk->tk_arg[0] = xe->xe_time;
		tk->tk_arg[1] = xe->xe_requestor;
		tk->tk_arg[2] = xe->xe_property;
		tk->tk_nargs = 3;
		break;
	case SelectionRequest :
		tk->tk_type = TK_SELREQUEST;
		tk->tk_arg[0] = xe->xe_time;
		tk->tk_arg[1] = xe->xe_requestor;
		tk->tk_arg[2] = xe->xe_target;
		tk->tk_arg[3] = xe->xe_property;
		tk->tk_nargs = 4;
		break;
	case ButtonPress :
		handle_button_press(tk, xe);
		break;
	case ButtonRelease :
		handle_button_release(tk, xe);
		break;
	case MotionNotify :
		handle_motion_notify(tk, xe);
		break;
	}
	free(xe);
	return true;
}

