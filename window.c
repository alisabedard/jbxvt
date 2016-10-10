/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "window.h"

#include "jbxvt.h"
#include "scr_reset.h"
#include "screen.h"

#include <stdlib.h>
#include <string.h>

//  Map the window
void jbxvt_map_window(void)
{
	xcb_map_window(jbxvt.X.xcb, jbxvt.X.win.main);
	xcb_map_subwindows(jbxvt.X.xcb, jbxvt.X.win.main);
	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.  */
	jbxvt_resize_window();
	jbxvt_reset(); // update size
}

static struct JBDim get_geometry(void)
{
	xcb_get_geometry_cookie_t c = xcb_get_geometry(jbxvt.X.xcb,
		jbxvt.X.win.main);
	xcb_generic_error_t * e;
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(jbxvt.X.xcb,
		c, &e);
	if (e)
		abort();
	struct JBDim ret = {.w = r->width, .height = r->height };
	free(r);
	// Resize area to fit characters
	ret.w -= ret.w % jbxvt.X.f.size.w;
	ret.h -= ret.h % jbxvt.X.f.size.h;
	return ret;
}

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void jbxvt_resize_window(void)
{
	struct JBDim p = get_geometry();
#define XCW(i) XCB_CONFIG_WINDOW_##i
	if (jbxvt.opt.show_scrollbar)
		p.width -= JBXVT_SCROLLBAR_WIDTH;
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.sb, XCW(HEIGHT),
		&(uint32_t){p.height});
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt, XCW(WIDTH)
		| XCW(HEIGHT), (uint32_t[]){p.w, p.h});
#undef XCW
	jbxvt.scr.chars = jbxvt_get_char_size(jbxvt.scr.pixels = p);
}

// Set main window property string
void jbxvt_set_property(const xcb_atom_t prop, const size_t sz,
	uint8_t * value)
{
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		jbxvt.X.win.main, prop, XCB_ATOM_STRING, 8, sz, value);
}

// Change window or icon name:
void jbxvt_change_name(uint8_t * restrict str, const bool icon)
{
	jbxvt_set_property(icon ? XCB_ATOM_WM_ICON_NAME
		: XCB_ATOM_WM_NAME, strlen((const char *)str), str);
}

