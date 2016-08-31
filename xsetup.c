/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xsetup.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "scr_reset.h"
#include "screen.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

#define VT jbxvt.X.win.vt
#define MW jbxvt.X.win.main
#define XC jbxvt.X.xcb
#define SW jbxvt.X.win.sb

//  Map the window
void map_window(void)
{
	xcb_map_window(XC, jbxvt.X.win.main);
	xcb_map_subwindows(XC, jbxvt.X.win.main);
	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.  */
	resize_window();
	scr_reset(); // update size
}

#define RSZ_VM (XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT)

static void cfg(const xcb_window_t win, const struct JBDim sz)
{
	xcb_configure_window(XC, win, RSZ_VM,
		(uint32_t[]){sz.w, sz.h});
}
#undef RSZ_VM

static int16_t resize_with_scrollbar(xcb_get_geometry_reply_t * r)
{
	// -1 to show the border:
	cfg(SW, (struct JBDim){.w = SBAR_WIDTH - 1, .h = r->height});
	cfg(VT, (struct JBDim){.w = r->width - SBAR_WIDTH,
		.h = r->height});
	return r->width - SBAR_WIDTH;
}

static int16_t resize_without_scrollbar(xcb_get_geometry_reply_t * r)
{
	cfg(jbxvt.X.win.vt, (struct JBDim){.w = r->width, .h = r->height});
	return r->width;
}

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void resize_window(void)
{
	xcb_get_geometry_cookie_t c = xcb_get_geometry(XC, MW);
	struct JBDim * ws = &jbxvt.X.window_size;
	xcb_get_geometry_reply_t *r = xcb_get_geometry_reply(XC, c, NULL);
	jb_assert(r, "Could not get geometry");
	if (r->width == ws->w && r->height == ws->h) {
		free(r);
		return; // size not changed.
	}
	ws->w = r->width;
	ws->h = r->height;
	jbxvt.scr.pixels.w = (jbxvt.opt.show_scrollbar
		? &resize_with_scrollbar : &resize_without_scrollbar)(r);
	jbxvt.scr.pixels.h = r->height;
	jbxvt.scr.chars = get_c(jbxvt.scr.pixels);
	free(r);
}

//  Toggle scrollbar.
void switch_scrollbar(void)
{
#define SB jbxvt.opt.show_scrollbar
	xcb_get_geometry_cookie_t c = xcb_get_geometry(XC, MW);
	uint16_t o = SB ? 0 : SBAR_WIDTH;
	xcb_configure_window(XC, VT, XCB_CONFIG_WINDOW_X, &o);
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(XC, c, NULL);
	uint16_t w = r->width;
	free(r);
	if (SB)
		w -= SBAR_WIDTH;
	xcb_configure_window(XC, MW, XCB_CONFIG_WINDOW_WIDTH, &w);
	SB ^= true;
}

// Change window or icon name:
void change_name(uint8_t * restrict str, const bool icon)
{
#define XA(n) XCB_ATOM_##n
	xcb_change_property(XC, XCB_PROP_MODE_REPLACE, MW, icon
		? XA(WM_ICON_NAME) : XA(WM_NAME), XA(STRING), 8,
		strlen((char*)str), str);
}

