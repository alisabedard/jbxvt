/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xsetup.h"

#include "config.h"
#include "init_display.h"
#include "jbxvt.h"
#include "log.h"
#include "scr_reset.h"
#include "screen.h"
#include "sbar.h"
#include "ttyinit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

//  Map the window
void map_window(void)
{
	XMapWindow(jbxvt.X.dpy, jbxvt.X.win.main);
	XMapSubwindows(jbxvt.X.dpy, jbxvt.X.win.main);
	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.  */
	XMaskEvent(jbxvt.X.dpy,ExposureMask,&(XEvent){});
	resize_window();
}

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void resize_window(void)
{
	LOG("resize_window()");
	// Quit before being messed up with a bad size:
	if (!jbxvt.scr.chars.width || !jbxvt.scr.chars.height)
		quit(1, WARN_ERR);
	unsigned int width, height;
	XGetGeometry(jbxvt.X.dpy, jbxvt.X.win.main, &(Window){0},
		&(int){0}, &(int){0}, &width, &height, &(unsigned int){0},
		&(unsigned int){0});
	if (jbxvt.opt.show_scrollbar) {
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.sb,
			SBAR_WIDTH - 1,height);
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.vt,
			width - SBAR_WIDTH,height);
	} else
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.vt,width,height);
	scr_reset();
}

//  Toggle scrollbar.
void switch_scrollbar(void)
{
	LOG("switch_scrollbar()");
	unsigned int width, height;
	XGetGeometry(jbxvt.X.dpy, jbxvt.X.win.main, &(Window){0},
		&(int){0}, &(int){0}, &width, &height,
		&(unsigned int){0}, &(unsigned int){0});
	if (jbxvt.opt.show_scrollbar) {
		XMoveWindow(jbxvt.X.dpy, jbxvt.X.win.vt, 0, 0);
		width -= SBAR_WIDTH;
	} else {
		XMoveWindow(jbxvt.X.dpy, jbxvt.X.win.vt, SBAR_WIDTH, 0);
		width += SBAR_WIDTH;
	}
	XResizeWindow(jbxvt.X.dpy, jbxvt.X.win.main, width, height);
	jbxvt.opt.show_scrollbar ^= true;
}

// Change window and/or icon name:
void change_name(uint8_t * restrict str, const bool window,
	const bool icon)
{
	LOG("change_name(%s, %d, %d)", str, window, icon);
	XTextProperty name;

	if (XStringListToTextProperty((char **)&str,1,&name)) {
		if(window)
			XSetWMName(jbxvt.X.dpy,jbxvt.X.win.main,&name);
		if(icon)
			XSetWMIconName(jbxvt.X.dpy,jbxvt.X.win.main,&name);
		XFree(name.value);
	}

}

