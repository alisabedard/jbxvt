/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "display.h"
#include "color.h"
#include "cursor.h"
#include "font.h"
#include "jbxvt.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "window.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <X11/cursorfont.h>
#include <xcb/xcb_icccm.h>
#define E(n) XCB_EVENT_MASK_##n
#define EB(n) XCB_EVENT_MASK_BUTTON_##n
enum EventMasks {
	MW_EVENTS = E(KEY_PRESS) | E(FOCUS_CHANGE) | E(STRUCTURE_NOTIFY),
	SUB_EVENTS = E(EXPOSURE) | EB(PRESS) | EB(RELEASE) | EB(MOTION)
};
static void create_main_window(xcb_connection_t * xc,
	xcb_size_hints_t * restrict sh, const xcb_window_t root)
{
	xcb_create_window(xc, 0, jbxvt_get_main_window(xc), root,
		sh->x, sh->y, sh->width, sh->height, 0, 0, 0,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(uint32_t[]){jbxvt_get_bg(), MW_EVENTS});
	jbxvt.scr.pixels.w = sh->width;
	jbxvt.scr.pixels.h = sh->height;
	jbxvt.scr.chars = jbxvt_get_char_size(jbxvt.scr.pixels);
}
static void create_sb_window(xcb_connection_t * xc, const uint16_t height)
{
	xcb_cursor_t c = jbxvt_get_cursor(xc,
		XC_sb_v_double_arrow, 0, 0xffff);
	xcb_create_window(xc, 0, jbxvt_get_scrollbar(xc),
		jbxvt_get_main_window(xc), -1, -1, JBXVT_SCROLLBAR_WIDTH - 1,
		height, 1, 0, 0, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL
		| XCB_CW_EVENT_MASK | XCB_CW_CURSOR, (uint32_t[]){
		jbxvt_get_bg(), jbxvt_get_fg(), SUB_EVENTS, c});
	xcb_free_cursor(xc, c);
}
static void create_vt_window(xcb_connection_t * xc,
	xcb_size_hints_t * restrict sh)
{
	xcb_cursor_t c = jbxvt_get_cursor(xc, XC_xterm, 0xffff, 0);
	xcb_create_window(xc, 0, jbxvt_get_vt_window(xc),
		jbxvt_get_main_window(xc), jbxvt.opt.show_scrollbar
		? JBXVT_SCROLLBAR_WIDTH : 0, 0, sh->width, sh->height,
		0, 0, 0, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK
		| XCB_CW_CURSOR, (uint32_t[]){ jbxvt_get_bg(),
		SUB_EVENTS, c});
	xcb_free_cursor(xc, c);
}
static void get_sizehints(xcb_size_hints_t * restrict s, struct JBDim p)
{
	p = jbxvt_get_pixel_size(p);
	*s = (xcb_size_hints_t) {
#define SH(n) XCB_ICCCM_SIZE_HINT_##n
		.flags = SH(US_SIZE) | SH(P_MIN_SIZE) | SH(P_RESIZE_INC)
			| SH(BASE_SIZE),
		.width = p.w, .height = p.h,
		.width_inc = jbxvt_get_font_size().w,
		.height_inc = jbxvt_get_font_size().h,
		.base_width = jbxvt_get_font_size().w,
		.base_height = jbxvt_get_font_size().h
	};
	s->min_width = jbxvt_get_font_size().w + s->base_width;
	s->min_height = jbxvt_get_font_size().h + s->base_height;
}
//  Open the window.
static void create_window(xcb_connection_t * xc, uint8_t * restrict name,
	const xcb_window_t root, struct JBDim size)
{
	xcb_size_hints_t sh;
	get_sizehints(&sh, size);
	create_main_window(xc, &sh, root);
	jbxvt_change_name(xc, name, true);
	jbxvt_change_name(xc, name, false);
	create_sb_window(xc, sh.height);
	create_vt_window(xc, &sh);
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
	const struct JBDim size, int * screen)
{
	xcb_connection_t * xc = jb_get_xcb_connection(NULL, screen);
	jbxvt_init_colors(xc);
	jbxvt_setup_fonts(xc);
	create_window(xc, (uint8_t *)name,
		jbxvt_get_root_window(xc), size);
	setup_gcs(xc, jbxvt_get_vt_window(xc));
	return xc;
}
