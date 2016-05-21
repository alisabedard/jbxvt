/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "wm_del_win.h"

#include "jbxvt.h"

Atom wm_del_win(void)
{
	static Atom a;
	if(!a) { // Init on first call:
		a = XInternAtom(jbxvt.X.dpy, "WM_DELETE_WINDOW", False);
		//  Enable the delete window protocol:
		XSetWMProtocols(jbxvt.X.dpy, jbxvt.X.win.main, &a, 1);
	}
	return a;
}

