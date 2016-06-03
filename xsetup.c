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

#ifdef USE_XCB
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#else//!USE_XCB
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif//USE_XCB

//  Map the window
void map_window(void)
{
#ifdef USE_XCB
	XMapWindow(jbxvt.X.dpy, jbxvt.X.win.main);
	// FIXME:  window must be created with xcb:
	//xcb_map_window(jbxvt.X.xcb, jbxvt.X.win.main);
	xcb_map_subwindows(jbxvt.X.xcb, jbxvt.X.win.main);
#else//!USE_XCB
	XMapWindow(jbxvt.X.dpy, jbxvt.X.win.main);
	XMapSubwindows(jbxvt.X.dpy, jbxvt.X.win.main);
#endif//USE_XCB
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
#ifdef USE_XCB
	xcb_get_geometry_cookie_t c = xcb_get_geometry(jbxvt.X.xcb,
		jbxvt.X.win.main);
	xcb_get_geometry_reply_t *r = xcb_get_geometry_reply(jbxvt.X.xcb,
		c, NULL);
	if (!r)
		  return;
	if (jbxvt.opt.show_scrollbar) {
		xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.sb,
			XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT,
			(uint32_t[]){SBAR_WIDTH - 1, r->height});
		xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt,
			XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT,
			(uint32_t[]){r->width - SBAR_WIDTH, r->height});
	} else {
		xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt,
			XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT,
			(uint32_t[]){r->width, r->height});
	}
	free(r);
#else//!USE_XCB
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
#endif//USE_XCB
	scr_reset();
}

//  Toggle scrollbar.
void switch_scrollbar(void)
{
	LOG("switch_scrollbar()");
#ifdef USE_XCB
	xcb_get_geometry_cookie_t c = xcb_get_geometry(jbxvt.X.xcb,
		jbxvt.X.win.main);
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt, XCB_CONFIG_WINDOW_X,
		(uint32_t[]){jbxvt.opt.show_scrollbar?0:SBAR_WIDTH});
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(jbxvt.X.xcb,
		c, NULL);
	if (!r)
		  return;
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.main,
		XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]){
		r->width+(jbxvt.opt.show_scrollbar?-SBAR_WIDTH:SBAR_WIDTH)});
#else//!USE_XCB
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
#endif//USE_XCB
	jbxvt.opt.show_scrollbar ^= true;
}

// Change window or icon name:
void change_name(uint8_t * restrict str, const bool icon)
{
#ifdef USE_XCB
	size_t l = 0;
	while(str[++l]);
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		jbxvt.X.win.main, icon?XCB_ATOM_WM_ICON_NAME:XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING, 8, l, str);
#else//!USE_XCB
	LOG("change_name(%s, %d, %d)", str, window, icon);
	XTextProperty name;

	if (XStringListToTextProperty((char **)&str,1,&name)) {
		if(icon)
			XSetWMIconName(jbxvt.X.dpy,jbxvt.X.win.main,&name);
		else
			XSetWMName(jbxvt.X.dpy,jbxvt.X.win.main,&name);
		XFree(name.value);
	}
#endif//USE_XCB
}

