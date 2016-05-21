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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static bool logshell;		/* flag nonzero if using a login shell */

XSizeHints sizehints = {
	PMinSize | PResizeInc | PBaseSize,
	0, 0, 80, 24,	/* x, y, width and height */
	1, 1,		/* Min width and height */
	0, 0,		/* Max width and height */
	1, 1,		/* Width and height increments */
	{0, 0}, {0, 0},	/* Aspect ratio - not used */
	2 * MARGIN, 2 * MARGIN,		/* base size */
	0
};

//  Return true if we should be running a login shell.
inline bool is_logshell(void)
{
	return(logshell);
}

//  Map the window
void map_window(void)
{
	XMapWindow(jbxvt.X.dpy,jbxvt.X.win.vt);
	XMapWindow(jbxvt.X.dpy,jbxvt.X.win.sb); // show scrollbar
	XMapWindow(jbxvt.X.dpy,jbxvt.X.win.main);

	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.
	 */
	XMaskEvent(jbxvt.X.dpy,ExposureMask,&(XEvent){});
	resize_window();
}

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows and return 1.  If the window
 *  size has not changed then return 0;
 */
int resize_window(void)
{
	LOG("resize_window()");
	int x, y;
	unsigned int width, height;
	{
		Window r;
		unsigned int u;
		XGetGeometry(jbxvt.X.dpy,jbxvt.X.win.main,&r,
			&x,&y,&width,&height,&u,&u);
	}
	if (jbxvt.opt.show_scrollbar) {
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.sb,SBAR_WIDTH - 1,height);
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.vt,width - SBAR_WIDTH,height);
	} else
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.vt,width,height);
	scr_reset();
	return(1);
}

//  Toggle scrollbar.
void switch_scrollbar(void)
{
	LOG("switch_scrollbar()");
	Window root;
	int x, y;
	unsigned int width, height, bdr_width, depth;

	XGetGeometry(jbxvt.X.dpy,jbxvt.X.win.main,&root,
		&x,&y,&width,&height,&bdr_width,&depth);
	if (jbxvt.opt.show_scrollbar) {
		XUnmapWindow(jbxvt.X.dpy,jbxvt.X.win.sb);
		XMoveWindow(jbxvt.X.dpy,jbxvt.X.win.vt,0,0);
		width -= SBAR_WIDTH;
		sizehints.base_width -= SBAR_WIDTH;
		sizehints.width = width;
		sizehints.height = height;
		sizehints.flags = USSize | PMinSize | PResizeInc | PBaseSize;
		XSetWMNormalHints(jbxvt.X.dpy,jbxvt.X.win.main,&sizehints);
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.main,width,height);
	} else {
		XMapWindow(jbxvt.X.dpy,jbxvt.X.win.sb);
		XMoveWindow(jbxvt.X.dpy,jbxvt.X.win.vt,SBAR_WIDTH,0);
		width += SBAR_WIDTH;
		sizehints.base_width += SBAR_WIDTH;
		sizehints.width = width;
		sizehints.height = height;
		sizehints.flags = USSize | PMinSize | PResizeInc | PBaseSize;
		XSetWMNormalHints(jbxvt.X.dpy,jbxvt.X.win.main,&sizehints);
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.main,width,height);
	}
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

