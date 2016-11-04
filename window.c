/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "window.h"
#include "font.h"
#include "jbxvt.h"
#include "sbar.h"
#include "scr_reset.h"
#include "screen.h"
#include "size.h"
#include <stdlib.h>
#include <string.h>
xcb_window_t jbxvt_get_main_window(xcb_connection_t * xc)
{
	static xcb_window_t w;
	if (w)
		return w;
	return w = xcb_generate_id(xc);
}
xcb_window_t jbxvt_get_vt_window(xcb_connection_t * xc)
{
	static xcb_window_t w;
	if (w)
		return w;
	return w = xcb_generate_id(xc);
}
//  Map the window
void jbxvt_map_window(xcb_connection_t * xc)
{
	xcb_map_window(xc, jbxvt_get_main_window(xc));
	xcb_map_subwindows(xc, jbxvt_get_main_window(xc));
	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.  */
	jbxvt_resize_window(xc);
	jbxvt_reset(xc); // update size
}
static struct JBDim get_geometry(xcb_connection_t * xc)
{
	xcb_get_geometry_cookie_t c = xcb_get_geometry(xc,
		jbxvt_get_main_window(xc));
	xcb_generic_error_t * e;
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(xc,
		c, &e);
	if (e)
		abort();
	struct JBDim ret = {.w = r->width, .height = r->height };
	free(r);
	// Resize area to fit characters
	const struct JBDim f = jbxvt_get_font_size();
	ret.w -= ret.w % f.w;
	ret.h -= ret.h % f.h;
	return ret;
}
/*  Called after a possible window size change.  If the window size
    has changed initiate a redraw by resizing the subwindows. */
void jbxvt_resize_window(xcb_connection_t * xc)
{
	struct JBDim p = get_geometry(xc);
#define XCW(i) XCB_CONFIG_WINDOW_##i
	if (jbxvt_get_scrollbar_visible())
		p.width -= JBXVT_SCROLLBAR_WIDTH;
	xcb_configure_window(xc, jbxvt_get_scrollbar(xc),
		XCW(HEIGHT), &(uint32_t){p.height});
	xcb_configure_window(xc, jbxvt_get_vt_window(xc), XCW(WIDTH)
		| XCW(HEIGHT), (uint32_t[]){p.w, p.h});
#undef XCW
	jbxvt_set_pixel_size(p);
}
// Set main window property string
void jbxvt_set_property(xcb_connection_t * xc, const xcb_atom_t prop,
	const size_t sz, uint8_t * value)
{
	xcb_change_property(xc, XCB_PROP_MODE_REPLACE,
		jbxvt_get_main_window(xc), prop, XCB_ATOM_STRING, 8, sz, value);
}
// Change window or icon name:
void jbxvt_change_name(xcb_connection_t * xc,
	uint8_t * restrict str, const bool icon)
{
	jbxvt_set_property(xc, icon ? XCB_ATOM_WM_ICON_NAME
		: XCB_ATOM_WM_NAME, strlen((const char *)str), str);
}
