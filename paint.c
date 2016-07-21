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
__attribute__((nonnull))
pixel_t get_pixel(const char * restrict color)
{
	return jb_get_pixel(jbxvt.X.xcb, jbxvt.X.screen->default_colormap,
		color);
}


void set_fg_or_bg(const char * color, const bool is_fg)
{
	*(is_fg ? &jbxvt.X.color.current_fg : & jbxvt.X.color.current_bg)
		= (is_fg ? jb_set_fg : jb_set_bg)(jbxvt.X.xcb,
		jbxvt.X.gc.tx, color ? get_pixel(color) : is_fg
		? jbxvt.X.color.fg : jbxvt.X.color.bg);
}

// 9-bit color
static pixel_t rgb_pixel(const uint16_t c)
{
	// Mask and scale to 8 bits.
	uint16_t r = (c & 0700) >> 6;
	uint16_t g = (c & 07) >> 3;
	uint16_t b = c & 7;
	const uint8_t o = 13; // scale, leave 3 bits for octal
	// Convert from 3 bit to 16 bit:
	r <<= o; g <<= o; b <<= o;
	// Compensate for bit limitation by increasing intensity:
	const uint16_t m = 017777;
	r |= m; g |= m; b |= m;
	const pixel_t p = jb_get_rgb_pixel(jbxvt.X.xcb,
		jbxvt.X.screen->default_colormap, r, g, b);
	CLOG("byte is 0x%x, r: 0x%x, g: 0x%x, b: 0x%x,"
		" pixel is 0x%x", c, r, g, b, p);
	return p;
}

static void fg(const pixel_t p)
{
	jbxvt.X.color.current_fg = jb_set_fg(jbxvt.X.xcb, jbxvt.X.gc.tx, p);
}

static void bg(const pixel_t p)
{
	jbxvt.X.color.current_bg = jb_set_bg(jbxvt.X.xcb, jbxvt.X.gc.tx, p);
}

static void rgb_fg(const uint16_t c)
{
	fg(rgb_pixel(c));
}

static void rgb_bg(const uint16_t c)
{
	bg(rgb_pixel(c));
}

static void ind_fg(const uint8_t index)
{
	fg(color_index[index]);
}

static void ind_bg(const uint8_t index)
{
	bg(color_index[index]);
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
	if (fg_rgb_mode) {
		CLOG("fg_rgb_mode: %d", bf);
		rgb_fg(bf);
//		set_rgb_colors(bf, true);
		fg_set = true;
	} else if (fg_index_mode) {
		CLOG("fg_index_mode: %d", bf);
		ind_fg(bf);
		fg_set = true;
	}

	if (bg_rgb_mode) {
		CLOG("bg_rgb_mode: %d", bb);
		rgb_bg(bb);
//		set_rgb_colors(bb, false);
		bg_set = true;
	} else if (bg_index_mode) {
		CLOG("bg_index_mode: %d", bb);
		ind_bg(bb);
		bg_set = true;
	}

	return fg_set || bg_set;
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
	if (rvid) { // Reverse looked up colors.
		LOG("rvid");
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx,
			XCB_GC_FOREGROUND | XCB_GC_BACKGROUND,
			(uint32_t[]){ jbxvt.X.color.current_bg,
			jbxvt.X.color.current_fg});
		cmod = true;
	}
	p.y += jbxvt.X.font_ascent;
	if(bold) {
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx,
			XCB_GC_FONT, &(uint32_t){
			jbxvt.X.bold_font});
	}
	// Draw text with background:
	xcb_image_text_8(jbxvt.X.xcb, len, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, p.x, p.y, (const char *)str);
	++p.y; /* Padding for underline,
		  use underline for italic. */
	if (rval & RS_ULINE || unlikely(rval & RS_ITALIC)) {
		xcb_poly_line(jbxvt.X.xcb, XCB_COORD_MODE_ORIGIN,
			jbxvt.X.win.vt, jbxvt.X.gc.tx, 2,
			(xcb_point_t[]){{p.x, p.y},
			{p.x + len * jbxvt.X.font_size.width,
			p.y}});
	}
	if(bold) { // restore font
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx,
			XCB_GC_FONT, &(uint32_t){jbxvt.X.font});
	}
	if (cmod) {
		set_fg(NULL);
		set_bg(NULL);
	}
}

