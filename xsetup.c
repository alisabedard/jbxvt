/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xsetup.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "scr_reset.h"
#include "screen.h"

#include <assert.h>
#include <string.h>

#define VT jbxvt.X.win.vt
#define MW jbxvt.X.win.main
#define XC jbxvt.X.xcb
#define SW jbxvt.X.win.sb
#define SB jbxvt.opt.show_scrollbar

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
	jbxvt_reset(); // update size
}

/*  Called after a possible window size change.  If the window size has changed
 *  initiate a redraw by resizing the subwindows. */
void resize_window(void)
{
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(XC,
		xcb_get_geometry(XC, MW), NULL);
	assert(r);
	uint32_t sz[] = {r->width, r->height};
	free(r);
	if (SB) {
#define XCW(i) XCB_CONFIG_WINDOW_##i
		xcb_configure_window(XC, SW, XCW(HEIGHT), &sz[1]);
		sz[0] -= SBAR_WIDTH;
	}
	xcb_configure_window(XC, VT, XCW(WIDTH) | XCW(HEIGHT), sz);
	CSZ = jbxvt_get_char_size(PSZ = (struct JBDim){
		.w = (uint16_t)sz[0],
		.h = (uint16_t)sz[1]});
}

// Change window or icon name:
void change_name(uint8_t * restrict str, const bool icon)
{
	assert(str);
#define XA(n) XCB_ATOM_##n
	xcb_change_property(XC, XCB_PROP_MODE_REPLACE, MW, icon
		? XA(WM_ICON_NAME) : XA(WM_NAME), XA(STRING), 8,
		strlen((char*)str), str);
}

