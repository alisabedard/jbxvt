/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "window.h"
#include <stdlib.h>
#include "JBXVTOptions.h"
#include "JBXVTToken.h"
#include "color.h"
#include "config.h"
#include "font.h"
#include "libjb/JBDim.h"
#include "libjb/util.h"
#include "libjb/xcb.h"
#include "sbar.h"
#include "scr_reset.h"
#include "size.h"
xcb_window_t jbxvt_get_main_window(xcb_connection_t * xc)
{
	static xcb_window_t w;
	return w ? w : (w = xcb_generate_id(xc));
}
xcb_window_t jbxvt_get_vt_window(xcb_connection_t * xc)
{
	static xcb_window_t w;
	return w ? w : (w = xcb_generate_id(xc));
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
static struct JBDim get_geometry_reply(xcb_connection_t * restrict xc,
	const xcb_get_geometry_cookie_t c)
{
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(xc, c, NULL);
	jb_require(r, "Could not get geometry");
	struct JBDim geo = {.w = r->width, .height = r->height };
	free(r);
	return geo;
}
static struct JBDim get_geometry(xcb_connection_t * restrict xc)
{
	struct JBDim geo = get_geometry_reply(xc,
		xcb_get_geometry(xc, jbxvt_get_main_window(xc)));
	// Resize area to fit characters
	const struct JBDim f = jbxvt_get_font_size();
	geo.w -= geo.w % f.w;
	geo.h -= geo.h % f.h;
	return geo;
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
void jbxvt_set_property(xcb_connection_t * xc, const xcb_atom_t property,
	const uint32_t data_len, uint8_t * data)
{
	xcb_change_property(xc, XCB_PROP_MODE_REPLACE,
		jbxvt_get_main_window(xc), property,
		XCB_ATOM_STRING, 8, data_len, data);
}
enum {
	CHILD_EVENT_MASK = XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_BUTTON_RELEASE
		| XCB_EVENT_MASK_BUTTON_MOTION,
	COPY_FROM_PARENT = XCB_COPY_FROM_PARENT
};
static void create_main_window(xcb_connection_t * xc,
	const xcb_window_t root, const struct JBDim position,
	const struct JBDim size)
{
	enum {
		VM = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		EMASK = XCB_EVENT_MASK_KEY_PRESS
			| XCB_EVENT_MASK_FOCUS_CHANGE
			| XCB_EVENT_MASK_STRUCTURE_NOTIFY,
		CFP = COPY_FROM_PARENT
	};
	xcb_create_window(xc, CFP, jbxvt_get_main_window(xc), root,
		position.x, position.y, size.width, size.height, 0, CFP,
		CFP, VM, (uint32_t[]){jbxvt_get_bg(), EMASK});
	jbxvt_set_pixel_size(size);
}
static void create_sb_window(xcb_connection_t * xc, const uint16_t height)
{
	enum {
		VM = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL
			| XCB_CW_EVENT_MASK | XCB_CW_CURSOR,
		CFP = COPY_FROM_PARENT,
		SB = JBXVT_SCROLLBAR_WIDTH
	};
	const xcb_cursor_t c = jb_get_cursor(xc,
		"sb_v_double_arrow");
	const xcb_rectangle_t r = {-1, -1, SB - 1, height};
	const xcb_window_t sb = jbxvt_get_scrollbar(xc),
	      mw = jbxvt_get_main_window(xc);
	xcb_create_window(xc, CFP, sb, mw, r.x, r.y, r.width, r.height,
		1, CFP, CFP, VM, (uint32_t[]){jbxvt_get_bg(),
		jbxvt_get_fg(), CHILD_EVENT_MASK, c});
	xcb_free_cursor(xc, c);
}
static void create_vt_window(xcb_connection_t * xc, const struct JBDim sz,
	const bool sb)
{
	enum {
		VM = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK
			| XCB_CW_CURSOR,
		CFP = COPY_FROM_PARENT,
		SB = JBXVT_SCROLLBAR_WIDTH
	};
	const xcb_cursor_t c = jb_get_cursor(xc, "xterm");
	const xcb_window_t vt = jbxvt_get_vt_window(xc),
	      mw = jbxvt_get_main_window(xc);
	const int16_t x = sb ? SB : 0;
	xcb_create_window(xc, CFP, vt, mw, x, 0, sz.w, sz.h, 0, CFP, CFP, VM,
		(uint32_t[]){jbxvt_get_bg(), CHILD_EVENT_MASK, c});
	xcb_free_cursor(xc, c);
}
// Change window or icon name:
static void chname(xcb_connection_t * xc,
	uint8_t * restrict str, const bool window, const bool icon)
{
	const xcb_window_t w = jbxvt_get_main_window(xc);
	char * cs = (char *)str;
	if (window)
		jb_set_window_name(xc, w, cs);
	if (icon)
		jb_set_icon_name(xc, w, cs);
}
// Create main window and the widget windows.
void jbxvt_create_window(xcb_connection_t * xc, const xcb_window_t root,
	struct JBXVTOptions * restrict opt, uint8_t * restrict name)
{
	struct JBDim sz = jbxvt_chars_to_pixels(opt->size);
	create_main_window(xc, root, opt->position, sz);
	create_sb_window(xc, sz.height);
	create_vt_window(xc, sz, opt->show_scrollbar);
	chname(xc, name, true, true);
}
// Handle the TXTPAR token to change the title bar or icon name.
void jbxvt_handle_JBXVT_TOKEN_TXTPAR(xcb_connection_t * xc, struct JBXVTToken
	* token)
{
	switch (token->arg[0]) {
	case 1 : // change icon name:
		chname(xc, token->string, false, true);
		break;
	case 2 : // change window name:
		chname(xc, token->string, true, false);
		break;
	default:
	case 0 : // change both:
		chname(xc, token->string, true, true);
		break;
	}
}
