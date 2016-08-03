/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "paint.h"

#include "color_index.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "screen.h"

#include <string.h>

#ifdef DEBUG_COLOR
#define CLOG(...) LOG(__VA_ARGS__)
#else
#define CLOG(...)
#endif

// returns pixel value for specified color
__attribute__((nonnull,pure))
static inline pixel_t pixel(const char * restrict color)
{
	return jb_get_pixel(jbxvt.X.xcb, jbxvt.X.screen->default_colormap,
		color);
}

static inline pixel_t set(pixel_t * restrict store,
	pixel_t (*func)(xcb_connection_t *,
	const xcb_gc_t, const pixel_t), const pixel_t p)
{
	return *store = func(jbxvt.X.xcb, jbxvt.X.gc.tx, p);
}

static inline pixel_t fg(const pixel_t p)
{
	return set(&jbxvt.X.color.current_fg, jb_set_fg, p);
}

static inline pixel_t bg(const pixel_t p)
{
	return set(&jbxvt.X.color.current_bg, jb_set_bg, p);
}

pixel_t set_fg(const char * color)
{
	return fg(color ? pixel(color) : jbxvt.X.color.fg);
}

pixel_t set_bg(const char * color)
{
	return bg(color ? pixel(color) : jbxvt.X.color.bg);
}

// 9-bit color
static pixel_t rgb_pixel(const uint16_t c)
{
	// Mask and scale to 8 bits.
	uint16_t r = (c & 0700) >> 4;
	uint16_t g = (c & 070) >> 2;
	uint16_t b = c & 7;

	//const uint8_t o = 13; // scale, leave 3 bits for octal
	const uint8_t o = 12; // scale, leave 3 bits for octal
	// Convert from 3 bit to 16 bit:
	r <<= o; g <<= o; b <<= o;
	const pixel_t p = jb_get_rgb_pixel(jbxvt.X.xcb,
		jbxvt.X.screen->default_colormap, r, g, b);
	LOG("byte is 0x%x, r: 0x%x, g: 0x%x, b: 0x%x,"
		" pixel is 0x%x", c, r, g, b, p);
	return p;
}

static bool set_rval_colors(const uint32_t rval)
{
	bool fg_rgb_mode = rval & RS_FG_RGB;
	bool bg_rgb_mode = rval & RS_BG_RGB;
	bool fg_index_mode = rval & RS_FG_INDEX;
	bool bg_index_mode = rval & RS_BG_INDEX;
	// Mask foreground colors, 9 bits offset by 6 bits
	uint8_t bf = rval >> 7;
	// Mask background colors, 9 bits offset by 15 bits
	uint8_t bb = rval >> 16;
	bool fg_set = false, bg_set = false;
	if ((fg_set = fg_rgb_mode))
		fg(rgb_pixel(bf));
	else if ((fg_set = fg_index_mode))
		fg(color_index[bf]);
	if ((bg_set = bg_rgb_mode))
		bg(rgb_pixel(bb));
	else if ((bg_set = bg_index_mode))
		bg(color_index[bb]);
	return fg_set || bg_set;
}

static inline void font(const xcb_font_t f)
{
	xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FONT, &f);
}

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, xcb_point_t p)
{
	if (!str || !len) // prevent segfault
		  return;
	if (rval & RS_INVISIBLE)
		  return; // nothing to do
	const bool rvid = (rval & RS_RVID) || (rval & RS_BLINK);
	const bool bold = rval & RS_BOLD;
	bool cmod = set_rval_colors(rval);
	xcb_connection_t * c = jbxvt.X.xcb;
	const xcb_gc_t gc = jbxvt.X.gc.tx;
	if (rvid) { // Reverse looked up colors.
		LOG("rvid");
		jb_set_fg(c, gc, jbxvt.X.color.current_bg);
		jb_set_bg(c, gc, jbxvt.X.color.current_fg);
		cmod = true;
	}
	p.y += jbxvt.X.f.ascent;
	if(bold)
		font(jbxvt.X.f.bold);
	// Draw text with background:
	const xcb_window_t w = jbxvt.X.win.vt;
	xcb_image_text_8(c, len, w, gc, p.x, p.y, (const char *)str);
	++p.y; /* Padding for underline,
		  use underline for italic. */
	if (rval & RS_ULINE || unlikely(rval & RS_ITALIC)) {
		xcb_poly_line(c, XCB_COORD_MODE_ORIGIN, w, gc, 2,
			(xcb_point_t[]){p, {p.x + len
			* jbxvt.X.f.size.width, p.y}});
	}
	if(bold) // restore font
		font(jbxvt.X.f.normal);
	if (cmod) {
		fg(jbxvt.X.color.fg);
		bg(jbxvt.X.color.bg);
	}
}

