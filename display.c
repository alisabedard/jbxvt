/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "display.h"
#include <stdlib.h>
#include <X11/cursorfont.h>
#include "color.h"
#include "config.h"
#include "cursor.h"
#include "paint.h"
#include "sbar.h"
#include "size.h"
#include "xcb_screen.h"
#include "window.h"
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
		CURSOR = XC_sb_v_double_arrow,
		VM = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL
			| XCB_CW_EVENT_MASK | XCB_CW_CURSOR,
		CFP = COPY_FROM_PARENT,
		SB = JBXVT_SCROLLBAR_WIDTH
	};
	const xcb_cursor_t c = jbxvt_get_xcb_cursor(xc, CURSOR, 0, 0xffff);
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
		CURSOR = XC_xterm,
		VM = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK
			| XCB_CW_CURSOR,
		CFP = COPY_FROM_PARENT,
		SB = JBXVT_SCROLLBAR_WIDTH
	};
	const xcb_cursor_t c = jbxvt_get_xcb_cursor(xc, XC_xterm, 0xffff, 0);
	const xcb_window_t vt = jbxvt_get_vt_window(xc),
	      mw = jbxvt_get_main_window(xc);
	const int16_t x = sb ? SB : 0;
	xcb_create_window(xc, CFP, vt, mw, x, 0, sz.w, sz.h, 0, CFP, CFP, VM,
		(uint32_t[]){jbxvt_get_bg(), CHILD_EVENT_MASK, c});
	xcb_free_cursor(xc, c);
}
static void set_name(xcb_connection_t * restrict xc,
	uint8_t * restrict name)
{
	jbxvt_change_name(xc, name, true);
	jbxvt_change_name(xc, name, false);
}
//  Open the window.
static void create_window(xcb_connection_t * xc, const xcb_window_t root,
	struct JBXVTOptions * restrict opt, uint8_t * restrict name)
{
	struct JBDim sz = jbxvt_chars_to_pixels(opt->size);
	create_main_window(xc, root, opt->position, sz);
	create_sb_window(xc, sz.height);
	create_vt_window(xc, sz, opt->show_scrollbar);
	set_name(xc, name);
}
static void setup_gcs(xcb_connection_t * xc, xcb_window_t w)
{
	enum {
		TXT_VM = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND
			| XCB_GC_FONT,
		CUR_VM = XCB_GC_FUNCTION | XCB_GC_PLANE_MASK
	};
	const pixel_t f = jbxvt_get_fg(), b = jbxvt_get_bg();
	xcb_create_gc(xc, jbxvt_get_text_gc(xc), w, TXT_VM,
		(uint32_t[]){f, b, jbxvt_get_normal_font(xc)});
	xcb_create_gc(xc, jbxvt_get_cursor_gc(xc), w, CUR_VM,
		(uint32_t[]){XCB_GX_INVERT, f ^ b});
}
xcb_connection_t * jbxvt_init_display(char * restrict name,
	struct JBXVTOptions * restrict opt)
{
	xcb_connection_t * xc;
	{ // screen scope
		int screen = opt->screen;
		xc = jb_get_xcb_connection(NULL, &screen);
		opt->screen = screen;
	}
	jbxvt_init_colors(xc, &opt->color);
	jbxvt_init_fonts(xc, &opt->font);
	create_window(xc, jbxvt_get_root_window(xc), opt, (uint8_t *)name);
	setup_gcs(xc, jbxvt_get_vt_window(xc));
	return xc;
}
