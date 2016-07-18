/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "init_display.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/xcb.h"
#include "sbar.h"
#include "screen.h"
#include "ttyinit.h"
#include "xsetup.h"

#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb_icccm.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

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
	xcb_font_t f = xcb_generate_id(jbxvt.X.xcb);
	size_t l = 0;
	while(name[++l]);
	xcb_void_cookie_t c = xcb_open_font_checked(jbxvt.X.xcb,
		f, l, name);
	xcb_generic_error_t * error
		= xcb_request_check(jbxvt.X.xcb, c);
	if (error) {
		l = 0;
		const char * fallback = FALLBACK_FONT;
		while(fallback[++l]);
		c = xcb_open_font_checked(jbxvt.X.xcb, f,
			l, fallback);
		error = xcb_request_check(jbxvt.X.xcb, c);
		if(jb_check(!error, "Could not open fallback font"))
			exit(1);
		if (jbxvt.X.font)
			  // Fall back if bold font
			  // unavailable:
			  jbxvt.X.bold_font = jbxvt.X.font;
	}
	return f;
}

static void setup_font(void)
{
	jbxvt.X.font = get_font(jbxvt.opt.font);
	xcb_query_font_cookie_t qfc = xcb_query_font(jbxvt.X.xcb,
		jbxvt.X.font);
	jbxvt.X.bold_font = get_font(jbxvt.opt.bold_font);
	xcb_query_font_reply_t * r
		= xcb_query_font_reply(jbxvt.X.xcb, qfc, NULL);
	jbxvt.X.font_ascent = r->font_ascent;
	jbxvt.X.font_size.width = r->max_bounds.character_width;
	jbxvt.X.font_size.height = r->font_ascent
		+ r->font_descent;
	free(r);
}

static void create_main_window(xcb_size_hints_t * restrict sh,
	const xcb_window_t root)
{
	jbxvt.X.win.main = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_window(jbxvt.X.xcb, XCB_COPY_FROM_PARENT,
		jbxvt.X.win.main, root, sh->x, sh->y,
		sh->width, sh->height, 0,
		XCB_WINDOW_CLASS_COPY_FROM_PARENT,
		XCB_COPY_FROM_PARENT, XCB_CW_EVENT_MASK,
		(uint32_t[]){MW_EVENTS});
	const Size f = jbxvt.X.font_size;
	Size * c = &jbxvt.scr.chars;
	Size * p = &jbxvt.scr.pixels;
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
	jbxvt.X.win.sb = xcb_generate_id(jbxvt.X.xcb);
	xcb_cursor_t c = get_cursor(XC_sb_v_double_arrow,
		0, 0xffff);
	xcb_create_window(jbxvt.X.xcb, XCB_COPY_FROM_PARENT,
		jbxvt.X.win.sb, jbxvt.X.win.main, -1, -1,
		SBAR_WIDTH - 1, height, 1,
		XCB_WINDOW_CLASS_COPY_FROM_PARENT,
		XCB_COPY_FROM_PARENT, XCB_CW_BACK_PIXEL
		| XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK
		| XCB_CW_CURSOR, (uint32_t[]){
		jbxvt.X.color.bg, jbxvt.X.color.fg,
		SUB_EVENTS, c});
	xcb_free_cursor(jbxvt.X.xcb, c);
}

static void create_vt_window(xcb_size_hints_t * restrict sh)
{
	jbxvt.X.win.vt = xcb_generate_id(jbxvt.X.xcb);
	xcb_cursor_t c = get_cursor(XC_xterm, 0xffff, 0);
	xcb_create_window(jbxvt.X.xcb, XCB_COPY_FROM_PARENT,
		jbxvt.X.win.vt, jbxvt.X.win.main, 0, 0,
		sh->width, sh->height, 0,
		XCB_WINDOW_CLASS_COPY_FROM_PARENT,
		XCB_COPY_FROM_PARENT, XCB_CW_BACK_PIXEL
		| XCB_CW_EVENT_MASK | XCB_CW_CURSOR,
		(uint32_t[]){jbxvt.X.color.bg, SUB_EVENTS, c});
	xcb_free_cursor(jbxvt.X.xcb, c);
}

static void get_sizehints(xcb_size_hints_t * restrict s)
{
	const Size f = jbxvt.X.font_size;
	*s = (xcb_size_hints_t) {
		.flags = USSize | PMinSize | PResizeInc | PBaseSize,
		.width = (1 + jbxvt.opt.size.width) * f.w,
		.height = (1 + jbxvt.opt.size.height) * f.h,
		.width_inc = f.w,
		.height_inc = f.h,
		.base_width = 300+f.w,
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

static void setup_gcs(void)
{
	jbxvt.X.gc.tx = xcb_generate_id(jbxvt.X.xcb);
	jbxvt.X.gc.cu = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_gc(jbxvt.X.xcb, jbxvt.X.gc.tx,
		jbxvt.X.win.main, XCB_GC_FOREGROUND
		| XCB_GC_BACKGROUND | XCB_GC_FONT,
		(uint32_t[]){jbxvt.X.color.fg,
		jbxvt.X.color.bg, jbxvt.X.font});
	xcb_create_gc(jbxvt.X.xcb, jbxvt.X.gc.cu,
		jbxvt.X.win.main, XCB_GC_FUNCTION
		| XCB_GC_PLANE_MASK, (uint32_t[]){XCB_GX_INVERT,
		jbxvt.X.color.cursor ^ jbxvt.X.color.bg});
}

static void init_jbxvt_colors(void)
{
	jbxvt.X.color.current_fg = jbxvt.X.color.fg
		= get_pixel(jbxvt.opt.fg);
	jbxvt.X.color.current_bg = jbxvt.X.color.bg
		= get_pixel(jbxvt.opt.bg);
	jbxvt.X.color.cursor = get_pixel(jbxvt.opt.cu);
}

void init_display(char * name)
{
	jbxvt.X.xcb = jb_get_xcb_connection(jbxvt.opt.display,
		&jbxvt.opt.screen);
	xcb_intern_atom_cookie_t c = xcb_intern_atom(jbxvt.X.xcb,
		false, 9, "CLIPBOARD");
	jbxvt.X.screen = jb_get_xcb_screen(jbxvt.X.xcb);
	const xcb_window_t root = jbxvt.X.screen->root;
	init_jbxvt_colors();
	setup_font();
	create_window((uint8_t *)name, root);
	setup_gcs();
	scr_init();
	xcb_intern_atom_reply_t * r
		= xcb_intern_atom_reply(jbxvt.X.xcb, c, NULL);
	jbxvt.X.clipboard = r->atom;
	free(r);
	cursor(CURSOR_FOCUS_IN);
}

