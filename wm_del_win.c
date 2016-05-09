#include "wm_del_win.h"

#include "jbxvt.h"

static Atom xa_wm_del_win;

void init_wm_del_win(void)
{
	xa_wm_del_win = XInternAtom(jbxvt.X.dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(jbxvt.X.dpy, jbxvt.X.win.main, &xa_wm_del_win, 1);
}

Atom get_wm_del_win(void)
{
	return xa_wm_del_win;
}

