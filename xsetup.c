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

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

//  Map the window
void map_window(void)
{
	xcb_map_window(jbxvt.X.xcb, jbxvt.X.win.main);
	xcb_map_subwindows(jbxvt.X.xcb, jbxvt.X.win.main);
	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.  */
	resize_window();
	// Forcibly show the window now:
	xcb_flush(jbxvt.X.xcb);
}

#define RSZ_VM (XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT)

static inline void cfg(const xcb_window_t win, const Size sz)
{
	xcb_configure_window(jbxvt.X.xcb, win, RSZ_VM,
		(uint32_t[]){sz.w, sz.h});
}

static inline void resize_with_scrollbar(xcb_get_geometry_reply_t * r)
{
	jbxvt.scr.pixels.width = r->width - SBAR_WIDTH;
	// -1 to show the border:
	cfg(jbxvt.X.win.sb, (Size){.w = SBAR_WIDTH - 1, .h = r->height});
	cfg(jbxvt.X.win.vt, (Size){.w = r->width - SBAR_WIDTH, .h = r->height});
}

static inline void resize_without_scrollbar(xcb_get_geometry_reply_t * r)
{
	jbxvt.scr.pixels.width = r->width;
	cfg(jbxvt.X.win.vt, (Size){.w = r->width, .h = r->height});
}
#undef RSZ_VM

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void resize_window(void)
{
	LOG("resize_window()");
	xcb_get_geometry_cookie_t c = xcb_get_geometry(jbxvt.X.xcb,
		jbxvt.X.win.main);
	xcb_get_geometry_reply_t *r = xcb_get_geometry_reply(jbxvt.X.xcb,
		c, NULL);
	if (!r) // Make sure reply was successful
		  return;
	(jbxvt.opt.show_scrollbar ? &resize_with_scrollbar
		: &resize_without_scrollbar)(r);
	jbxvt.scr.pixels.height = r->height;
	free(r);
	scr_reset();
}

//  Toggle scrollbar.
void switch_scrollbar(void)
{
	LOG("switch_scrollbar()");
	xcb_get_geometry_cookie_t c = xcb_get_geometry(jbxvt.X.xcb,
		jbxvt.X.win.main);
	const bool sb = jbxvt.opt.show_scrollbar;
	int16_t w = sb ? 0 : SBAR_WIDTH;
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt,
		XCB_CONFIG_WINDOW_X, &w);
	xcb_get_geometry_reply_t * r;
	r = xcb_get_geometry_reply(jbxvt.X.xcb, c, NULL);
	w = r->width + (sb ? -SBAR_WIDTH : w);
	free(r);
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.main,
		XCB_CONFIG_WINDOW_WIDTH, &w);
	jbxvt.opt.show_scrollbar = !sb;
}

// Change window or icon name:
void change_name(uint8_t * restrict str, const bool icon)
{
	size_t l = 0;
	while(str[++l]);
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		jbxvt.X.win.main, icon?XCB_ATOM_WM_ICON_NAME:XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING, 8, l, str);
}

