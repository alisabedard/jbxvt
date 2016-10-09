/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xsetup.h"

#include "jbxvt.h"
#include "scr_reset.h"
#include "screen.h"

#include <stdlib.h>

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

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void jbxvt_resize_window(void)
{
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(jbxvt.X.xcb,
		xcb_get_geometry(jbxvt.X.xcb, jbxvt.X.win.main), NULL);
	uint32_t sz[] = {r->width, r->height};
	free(r);
	if (jbxvt.opt.show_scrollbar) {
#define XCW(i) XCB_CONFIG_WINDOW_##i
		xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.sb,
			XCW(HEIGHT), &sz[1]);
		sz[0] -= JBXVT_SCROLLBAR_WIDTH;
	}
	xcb_configure_window(jbxvt.X.xcb, jbxvt.X.win.vt,
		XCW(WIDTH) | XCW(HEIGHT), sz);
	jbxvt.scr.chars = jbxvt_get_char_size(jbxvt.scr.pixels
		= (struct JBDim){ .w = (uint16_t)sz[0],
		.h = (uint16_t)sz[1]});
}

// Change window or icon name:
void jbxvt_change_name(uint8_t * restrict str, const bool icon)
{
#define XA(n) XCB_ATOM_##n
	uint16_t l = 0;
	while (str[++l]);
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		jbxvt.X.win.main, icon ? XA(WM_ICON_NAME) : XA(WM_NAME),
		XA(STRING), 8, l, str);
}

