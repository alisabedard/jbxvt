/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "display.h"
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
static xcb_font_t get_font(const char * name)
{
	errno = 0;
	xcb_font_t f = xcb_generate_id(jbxvt.X.xcb);
	xcb_void_cookie_t c = xcb_open_font_checked(jbxvt.X.xcb, f,
		strlen(name), name);
	if (jb_xcb_cookie_has_error(jbxvt.X.xcb, c)) {
		if (jbxvt.X.font.normal) // Fall back to normal font first
			return jbxvt.X.font.normal;
		c = xcb_open_font_checked(jbxvt.X.xcb, f, sizeof(FALLBACK_FONT),
			FALLBACK_FONT);
		jb_require(!jb_xcb_cookie_has_error(jbxvt.X.xcb, c),
			"Could not load any fonts");
	}
	return f;
}
static void setup_font_metrics(const xcb_query_font_cookie_t c)
{
	errno = 0;
	xcb_query_font_reply_t * r = xcb_query_font_reply(jbxvt.X.xcb,
		c, NULL);
	jb_assert(r, "Cannot get font information");
	jbxvt.X.font.ascent = r->font_ascent;
	jbxvt.X.font.size.width = r->max_bounds.character_width;
	jbxvt.X.font.size.height = r->font_ascent + r->font_descent;
	free(r);
}
static void setup_fonts(void)
{
	jbxvt.X.font.normal = get_font(jbxvt.opt.font);
	const xcb_query_font_cookie_t c = xcb_query_font(jbxvt.X.xcb,
		jbxvt.X.font.normal);
	jbxvt.X.font.bold = get_font(jbxvt.opt.bold_font);
	jbxvt.X.font.italic = get_font(jbxvt.opt.italic_font);
	setup_font_metrics(c);
}
static void create_main_window(xcb_size_hints_t * restrict sh,
	const xcb_window_t root)
{
	jbxvt.X.win.main = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_window(jbxvt.X.xcb, 0, jbxvt.X.win.main, root,
		sh->x, sh->y, sh->width, sh->height, 0, 0, 0,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(uint32_t[]){jbxvt.X.color.bg, MW_EVENTS});
	jbxvt.scr.pixels.w = sh->width;
	jbxvt.scr.pixels.h = sh->height;
	jbxvt.scr.chars = jbxvt_get_char_size(jbxvt.scr.pixels);
}
static void open_cursor(const xcb_font_t f)
{
	errno = 0;
	xcb_void_cookie_t v = xcb_open_font_checked(jbxvt.X.xcb, f, 6,
		"cursor");
	jb_require(!jb_xcb_cookie_has_error(jbxvt.X.xcb, v),
		"Cannot open cursor font");
}
static xcb_cursor_t get_cursor(const uint16_t id,
	const uint16_t fg, const uint16_t bg)
{
	xcb_font_t f = xcb_generate_id(jbxvt.X.xcb);
	open_cursor(f);
	xcb_cursor_t c = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_glyph_cursor(jbxvt.X.xcb, c, f, f,
		id, id + 1, fg, fg, fg, bg, bg, bg);
	xcb_close_font(jbxvt.X.xcb, f);
	return c;
}
static void create_sb_window(xcb_connection_t * xc, const uint16_t height)
{
	xcb_cursor_t c = get_cursor(XC_sb_v_double_arrow, 0, 0xffff);
	xcb_create_window(jbxvt.X.xcb, 0, jbxvt_get_scrollbar(xc),
		jbxvt.X.win.main, -1, -1, JBXVT_SCROLLBAR_WIDTH - 1,
		height, 1, 0, 0, XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL
		| XCB_CW_EVENT_MASK | XCB_CW_CURSOR, (uint32_t[]){
		jbxvt.X.color.bg, jbxvt.X.color.fg, SUB_EVENTS, c});
	xcb_free_cursor(jbxvt.X.xcb, c);
}
static void create_vt_window(xcb_size_hints_t * restrict sh)
{
	xcb_connection_t * xc = jbxvt.X.xcb;
	jbxvt.X.win.vt = xcb_generate_id(xc);
	xcb_cursor_t c = get_cursor(XC_xterm, 0xffff, 0);
	xcb_create_window(xc, 0, jbxvt.X.win.vt, jbxvt.X.win.main,
		jbxvt.opt.show_scrollbar ? JBXVT_SCROLLBAR_WIDTH : 0, 0,
		sh->width, sh->height, 0, 0, 0, XCB_CW_BACK_PIXEL
		| XCB_CW_EVENT_MASK | XCB_CW_CURSOR, (uint32_t[]){
		jbxvt.X.color.bg, SUB_EVENTS, c});
	xcb_free_cursor(xc, c);
}
static void get_sizehints(xcb_size_hints_t * restrict s)
{
	const struct JBDim p = jbxvt_get_pixel_size(jbxvt.opt.size);
	*s = (xcb_size_hints_t) {
#define SH(n) XCB_ICCCM_SIZE_HINT_##n
		.flags = SH(US_SIZE) | SH(P_MIN_SIZE) | SH(P_RESIZE_INC)
			| SH(BASE_SIZE),
		.width = p.w, .height = p.h,
#define FSZ jbxvt.X.font.size
		.width_inc = FSZ.w,
		.height_inc = FSZ.h,
		.base_width = FSZ.w,
		.base_height = FSZ.h
	};
	s->min_width = FSZ.w + s->base_width;
	s->min_height = FSZ.h + s->base_height;
#undef FSZ
}
//  Open the window.
static void create_window(xcb_connection_t * xc, uint8_t * restrict name,
	const xcb_window_t root)
{
	xcb_size_hints_t sh;
	get_sizehints(&sh);
	create_main_window(&sh, root);
	jbxvt_change_name(name, true);
	jbxvt_change_name(name, false);
	create_sb_window(xc, sh.height);
	create_vt_window(&sh);
}
static xcb_gc_t get_gc(const uint32_t vm, const void * vl)
{
	xcb_gc_t g = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_gc(jbxvt.X.xcb, g, jbxvt.X.win.main, vm, vl);
	return g;
}
static void setup_gcs(void)
{
	const pixel_t f = jbxvt.X.color.fg, b = jbxvt.X.color.bg;
	jbxvt.X.gc.tx = get_gc(XCB_GC_FOREGROUND | XCB_GC_BACKGROUND
		| XCB_GC_FONT, (uint32_t[]){f, b, jbxvt.X.font.normal});
	jbxvt.X.gc.cu = get_gc(XCB_GC_FUNCTION | XCB_GC_PLANE_MASK,
		(uint32_t[]){XCB_GX_INVERT, f ^ b});
}
static inline void init_jbxvt_colors(void)
{
	jbxvt.X.color.fg = jbxvt_set_fg(jbxvt.opt.fg);
	jbxvt.X.color.bg = jbxvt_set_bg(jbxvt.opt.bg);
}
xcb_connection_t * jbxvt_init_display(char * restrict name)
{
	int screen = jbxvt.opt.screen;
	xcb_connection_t * xc = jbxvt.X.xcb
		= jb_get_xcb_connection(jbxvt.opt.display, &screen);
	init_jbxvt_colors();
	setup_fonts();
	create_window(xc, (uint8_t *)name,
		jbxvt_get_root_window(jbxvt.X.xcb));
	setup_gcs();
	return xc;
}
