/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xsetup.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "scr_reset.h"

#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

//  Map the window
void map_window(void)
{
	xcb_connection_t * x = jbxvt.X.xcb;
	xcb_map_window(x, jbxvt.X.win.main);
	xcb_map_subwindows(x, jbxvt.X.win.main);
	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.  */
	resize_window();
	// Forcibly show the window now:
	xcb_flush(x);
}

#define RSZ_VM (XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT)

static void cfg(const xcb_window_t win, const Size sz)
{
	xcb_configure_window(jbxvt.X.xcb, win, RSZ_VM,
		(uint32_t[]){sz.w, sz.h});
}

static int16_t resize_with_scrollbar(xcb_get_geometry_reply_t * r)
{
	// -1 to show the border:
	cfg(jbxvt.X.win.sb, (Size){.w = SBAR_WIDTH - 1, .h = r->height});
	cfg(jbxvt.X.win.vt, (Size){.w = r->width - SBAR_WIDTH,
		.h = r->height});
	return r->width - SBAR_WIDTH;
}

static int16_t resize_without_scrollbar(xcb_get_geometry_reply_t * r)
{
	cfg(jbxvt.X.win.vt, (Size){.w = r->width, .h = r->height});
	return r->width;
}
#undef RSZ_VM

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void resize_window(void)
{
	xcb_get_geometry_cookie_t c = xcb_get_geometry(jbxvt.X.xcb,
		jbxvt.X.win.main);
	Size * ws = &jbxvt.X.window_size;
	xcb_get_geometry_reply_t *r = xcb_get_geometry_reply(jbxvt.X.xcb,
		c, NULL);
	if (jb_check(r, "Could not get geometry"))
		exit(1);
	if (r->width == ws->w && r->height == ws->h)
		  return; // Size has not changed.
	ws->w = r->width;
	ws->h = r->height;
	jbxvt.scr.pixels.w = (jbxvt.opt.show_scrollbar
		? &resize_with_scrollbar : &resize_without_scrollbar)(r);
	jbxvt.scr.pixels.h = r->height;
	free(r);
	scr_reset();
}

//  Toggle scrollbar.
void switch_scrollbar(void)
{
	LOG("switch_scrollbar()");
	xcb_connection_t * x = jbxvt.X.xcb;
	const xcb_window_t mw = jbxvt.X.win.main;
	xcb_get_geometry_cookie_t c = xcb_get_geometry(x, mw);
	const bool sb = jbxvt.opt.show_scrollbar;
	int16_t w = sb ? 0 : SBAR_WIDTH;
	xcb_configure_window(x, jbxvt.X.win.vt, XCB_CONFIG_WINDOW_X, &w);
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(x, c, NULL);
	if (jb_check(r, "Could not get geometry"))
		abort();
	w = r->width + (sb ? -SBAR_WIDTH : w);
	free(r);
	xcb_configure_window(x, mw, XCB_CONFIG_WINDOW_WIDTH, &w);
	jbxvt.opt.show_scrollbar ^= true;
}

// Change window or icon name:
void change_name(uint8_t * restrict str, const bool icon)
{
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		jbxvt.X.win.main, icon ? XCB_ATOM_WM_ICON_NAME
		: XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen((char*)str),
		str);
}

