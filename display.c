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
#include "screen.h"
#include "size.h"
#include "xcb_screen.h"
#include "window.h"
#define E(n) XCB_EVENT_MASK_##n
#define EB(n) XCB_EVENT_MASK_BUTTON_##n
enum EventMasks {
	JBXVT_MAIN_EVENT_MASK = E(KEY_PRESS) | E(FOCUS_CHANGE)
		| E(STRUCTURE_NOTIFY),
	JBXVT_CHILD_EVENT_MASK = E(EXPOSURE) | EB(PRESS) | EB(RELEASE)
		| EB(MOTION)
};
#undef E
#undef EB
static void create_main_window(xcb_connection_t * xc,
	const xcb_window_t root, const struct JBDim position,
	const struct JBDim size)
{
	xcb_create_window(xc, 0, jbxvt_get_main_window(xc), root, position.x,
		position.y, size.width, size.height, 0, 0, 0,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, (uint32_t[]){
		jbxvt_get_bg(), JBXVT_MAIN_EVENT_MASK});
	jbxvt_set_pixel_size(size);
}
static void create_sb_window(xcb_connection_t * xc, const uint16_t height)
{
	const xcb_cursor_t c = jbxvt_get_cursor(xc,
		XC_sb_v_double_arrow, 0, 0xffff);
	xcb_create_window(xc, 0, jbxvt_get_scrollbar(xc),
		jbxvt_get_main_window(xc), -1, -1, JBXVT_SCROLLBAR_WIDTH - 1,
		height, 1, 0, 0, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL
		| XCB_CW_EVENT_MASK | XCB_CW_CURSOR, (uint32_t[]){
		jbxvt_get_bg(), jbxvt_get_fg(), JBXVT_CHILD_EVENT_MASK, c});
	xcb_free_cursor(xc, c);
}
static void create_vt_window(xcb_connection_t * xc, const struct JBDim sz,
	const bool sb)
{
	const xcb_cursor_t c = jbxvt_get_cursor(xc, XC_xterm, 0xffff, 0);
	xcb_create_window(xc, 0, jbxvt_get_vt_window(xc),
		jbxvt_get_main_window(xc), sb ? JBXVT_SCROLLBAR_WIDTH : 0, 0,
		sz.w, sz.h, 0, 0, 0, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK
		| XCB_CW_CURSOR, (uint32_t[]){ jbxvt_get_bg(),
		JBXVT_CHILD_EVENT_MASK, c});
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
	const pixel_t f = jbxvt_get_fg(), b = jbxvt_get_bg();
	xcb_create_gc(xc, jbxvt_get_text_gc(xc), w,
		XCB_GC_FOREGROUND | XCB_GC_BACKGROUND
		| XCB_GC_FONT, (uint32_t[]){f, b,
		jbxvt_get_normal_font(xc)});
	xcb_create_gc(xc, jbxvt_get_cursor_gc(xc), w,
		XCB_GC_FUNCTION | XCB_GC_PLANE_MASK, (uint32_t[]){
		XCB_GX_INVERT, f ^ b});
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
