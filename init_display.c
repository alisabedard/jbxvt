/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "init_display.h"

#include "command.h"
#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/xcb.h"
#include "paint.h"
#include "sbar.h"
#include "xsetup.h"

#include <stdlib.h>
#include <string.h>
#include <xcb/xcb_icccm.h>
#include <X11/cursorfont.h>

enum EventMasks {
	MW_EVENTS = (XCB_EVENT_MASK_KEY_PRESS
		| XCB_EVENT_MASK_FOCUS_CHANGE
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY),
	SUB_EVENTS = (XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_BUTTON_RELEASE
		| XCB_EVENT_MASK_BUTTON_MOTION)
};

static xcb_font_t get_font(const char * name)
{
	xcb_connection_t * restrict x = jbxvt.X.xcb;
	xcb_font_t f = xcb_generate_id(x);
	xcb_void_cookie_t c = xcb_open_font_checked(x, f,
		strlen(name), name);
	xcb_generic_error_t * error = xcb_request_check(x, c);
	if (error) {
		free(error);
		c = xcb_open_font_checked(x, f, sizeof(FALLBACK_FONT),
			FALLBACK_FONT);
		error = xcb_request_check(x, c);
		if(jb_check(!error, "Could not open fallback font"))
			exit(1); // no need to free error, exiting
		if (jbxvt.X.f.normal) // already set
			  // Fall back if bold font unavailable:
			  jbxvt.X.f.bold = jbxvt.X.f.normal;
	}
	return f;
}

static void setup_font(void)
{
	struct JBXVTFontData * f = &jbxvt.X.f;
	xcb_query_font_cookie_t qfc = xcb_query_font(jbxvt.X.xcb,
		f->normal = get_font(jbxvt.opt.font));
	f->bold = get_font(jbxvt.opt.bold_font);
	xcb_query_font_reply_t * r = xcb_query_font_reply(jbxvt.X.xcb,
		qfc, NULL);
	f->ascent = r->font_ascent;
	struct JBDim * s = &f->size;
	s->width = r->max_bounds.character_width;
	s->height = r->font_ascent + r->font_descent;
	free(r);
}

static void create_main_window(xcb_size_hints_t * restrict sh,
	const xcb_window_t root)
{
	jbxvt.X.win.main = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_window(jbxvt.X.xcb, 0, jbxvt.X.win.main, root,
		sh->x, sh->y, sh->width, sh->height, 0, 0, 0,
		XCB_CW_EVENT_MASK,
		(uint32_t[]){MW_EVENTS});
	const struct JBDim f = jbxvt.X.f.size;
	struct JBDim * c = &jbxvt.scr.chars;
	struct JBDim * p = &jbxvt.scr.pixels;
	c->w = (p->w = sh->width) / f.w;
	c->h = (p->h = sh->height) / f.h;
}

static xcb_cursor_t get_cursor(const uint16_t id,
	const uint16_t fg, const uint16_t bg)
{
	xcb_font_t f = xcb_generate_id(jbxvt.X.xcb);
	xcb_open_font(jbxvt.X.xcb, f, 6, "cursor");
	xcb_cursor_t c = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_glyph_cursor(jbxvt.X.xcb, c, f, f,
		id, id + 1, fg, fg, fg, bg, bg, bg);
	xcb_close_font(jbxvt.X.xcb, f);
	return c;
}

static void create_sb_window(const uint16_t height)
{
	xcb_cursor_t c = get_cursor(XC_sb_v_double_arrow, 0, 0xffff);
	xcb_create_window(jbxvt.X.xcb, 0, jbxvt.X.win.sb
		= xcb_generate_id(jbxvt.X.xcb), jbxvt.X.win.main, -1, -1,
		SBAR_WIDTH - 1, height, 1, 0, 0, XCB_CW_BACK_PIXEL
		| XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK
		| XCB_CW_CURSOR, (uint32_t[]){ jbxvt.X.color.bg,
		jbxvt.X.color.fg, SUB_EVENTS, c});
	xcb_free_cursor(jbxvt.X.xcb, c);
}

static void create_vt_window(xcb_size_hints_t * restrict sh)
{
	xcb_connection_t * xc = jbxvt.X.xcb;
	jbxvt.X.win.vt = xcb_generate_id(xc);
	xcb_cursor_t c = get_cursor(XC_xterm, 0xffff, 0);
	xcb_create_window(xc, 0, jbxvt.X.win.vt, jbxvt.X.win.main, 0, 0,
		sh->width, sh->height, 0, 0, 0, XCB_CW_BACK_PIXEL
		| XCB_CW_EVENT_MASK | XCB_CW_CURSOR, (uint32_t[]){
		jbxvt.X.color.bg, SUB_EVENTS, c});
	xcb_free_cursor(xc, c);
}

static void get_sizehints(xcb_size_hints_t * restrict s)
{
	const struct JBDim f = jbxvt.X.f.size;
	*s = (xcb_size_hints_t) {
		.flags = XCB_ICCCM_SIZE_HINT_US_SIZE
			| XCB_ICCCM_SIZE_HINT_P_MIN_SIZE
			| XCB_ICCCM_SIZE_HINT_P_RESIZE_INC
			| XCB_ICCCM_SIZE_HINT_BASE_SIZE,
		.width = (2 + jbxvt.opt.size.width) * f.w, // Make 80
		.height = (1 + jbxvt.opt.size.height) * f.h, // Make 24
		.width_inc = f.w,
		.height_inc = f.h,
		.base_width = f.w,
		.base_height = f.h
	};
	s->min_width = f.w + s->base_width;
	s->min_height = f.h + s->base_height;
}

//  Open the window.
static void create_window(uint8_t * restrict name,
	const xcb_window_t root)
{
	xcb_size_hints_t sh;
	get_sizehints(&sh);
	create_main_window(&sh, root);
	change_name(name, true);
	change_name(name, false);
	create_sb_window(sh.height);
	create_vt_window(&sh);
	jbxvt.opt.show_scrollbar ^= true;
	switch_scrollbar();
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
		| XCB_GC_FONT, (uint32_t[]){f, b, jbxvt.X.f.normal});
	jbxvt.X.gc.cu = get_gc(XCB_GC_FUNCTION | XCB_GC_PLANE_MASK,
		(uint32_t[]){XCB_GX_INVERT, f ^ b});
}

static inline void init_jbxvt_colors(void)
{
	jbxvt.X.color.fg = set_fg(jbxvt.opt.fg);
	jbxvt.X.color.bg = set_bg(jbxvt.opt.bg);
}

void init_display(char * name)
{
	jbxvt.X.xcb = jb_get_xcb_connection(jbxvt.opt.display,
		(int*)&jbxvt.opt.screen);
	jbxvt.X.screen = jb_get_xcb_screen(jbxvt.X.xcb);
	init_jbxvt_colors();
	setup_font();
	create_window((uint8_t *)name, jbxvt.X.screen->root);
	setup_gcs();
	jbxvt.X.clipboard = jb_get_atom(jbxvt.X.xcb, "CLIPBOARD");
}

