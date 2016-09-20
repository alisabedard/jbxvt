/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xsetup.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
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
#define SB jbxvt.opt.show_scrollbar
#define CSZ jbxvt.scr.chars
#define PSZ jbxvt.scr.pixels

//  Map the window
void map_window(void)
{
	xcb_map_window(XC, MW);
	xcb_map_subwindows(XC, MW);
	if (SB)
		jbxvt_show_sbar();
	/*  Setup the window now so that we can add LINES and COLUMNS to
	 *  the environment.  */
	resize_window();
	scr_reset(); // update size
}

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void resize_window(void)
{
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(XC,
		xcb_get_geometry(XC, MW), NULL);
	struct JBDim sz = {.w = r->width, .h = r->height};
	free(r);
	if (SB) {
		xcb_configure_window(XC, SW, XCB_CONFIG_WINDOW_HEIGHT,
			(uint32_t[]){sz.h});
		sz.w -= SBAR_WIDTH;
	}
	xcb_configure_window(XC, VT, XCB_CONFIG_WINDOW_WIDTH |
		XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]){sz.w, sz.h});
	CSZ = jbxvt_get_char_size(PSZ = sz);
}

// Change window or icon name:
void change_name(uint8_t * restrict str, const bool icon)
{
#define XA(n) XCB_ATOM_##n
	xcb_change_property(XC, XCB_PROP_MODE_REPLACE, MW, icon
		? XA(WM_ICON_NAME) : XA(WM_NAME), XA(STRING), 8,
		strlen((char*)str), str);
}

