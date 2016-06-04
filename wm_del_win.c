/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "wm_del_win.h"

#include "jbxvt.h"

#include <stdlib.h>

long unsigned int wm_del_win(void)
{
	static long unsigned int a;
	if(!a) { // Init on first call:
#ifdef USE_XCB
		xcb_intern_atom_cookie_t c = xcb_intern_atom(
			jbxvt.X.xcb, false, 16, "WM_DELETE_WINDOW");
		xcb_intern_atom_cookie_t protoc = xcb_intern_atom(
			jbxvt.X.xcb, false, 12, "WM_PROTOCOLS");
		xcb_intern_atom_reply_t * r = xcb_intern_atom_reply(
			jbxvt.X.xcb, c, NULL);
		a = r->atom;
		free(r);
		r = xcb_intern_atom_reply(jbxvt.X.xcb, protoc, NULL);
		xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
			jbxvt.X.win.main, r->atom, XCB_ATOM_ATOM,
			32, 1, &a);
		free(r);
#else//!USE_XCB
		a = XInternAtom(jbxvt.X.dpy, "WM_DELETE_WINDOW", False);
		//  Enable the delete window protocol:
		XSetWMProtocols(jbxvt.X.dpy, jbxvt.X.win.main, &a, 1);
#endif//USE_XCB
	}
	return a;
}

