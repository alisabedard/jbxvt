/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "paint.h"

#include "color_index.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"

#include <string.h>

#ifdef DEBUG_COLOR
#define CLOG(...) LOG(__VA_ARGS__)
#else
#define CLOG(...)
#endif

static pixel_t get_pixel_for_word(const uint16_t c)
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
	pixel_t p = get_pixel_rgb(r, g, b);
	CLOG("byte is 0x%x, r: 0x%x, g: 0x%x, b: 0x%x,"
		" pixel is 0x%x", c, r, g, b, p);
	return p;
}

// Set 9-bit color
static pixel_t set_rgb_colors(const uint16_t c, const bool fg)
{
	pixel_t p = get_pixel_for_word(c);
	set_color(fg ? XCB_GC_FOREGROUND : XCB_GC_BACKGROUND,
		c, jbxvt.X.gc.tx);
	return p;
}

static pixel_t set_index_colors(const uint8_t index,
	const bool fg)
{
	const pixel_t p = color_index[index];
	set_color(fg ? XCB_GC_FOREGROUND : XCB_GC_BACKGROUND,
		p, jbxvt.X.gc.tx);
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
	if (fg_rgb_mode) {
		CLOG("fg_rgb_mode: %d", bf);
		set_rgb_colors(bf, true);
		fg_set = true;
	} else if (fg_index_mode) {
		CLOG("fg_index_mode: %d", bf);
		set_index_colors(bf, true);
		fg_set = true;
	}

	if (bg_rgb_mode) {
		CLOG("bg_rgb_mode: %d", bb);
		set_rgb_colors(bb, false);
		bg_set = true;
	} else if (bg_index_mode) {
		CLOG("bg_index_mode: %d", bb);
		set_index_colors(bb, false);
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
	xcb_flush(jbxvt.X.xcb);
	++p.y; /* Padding for underline,
		  use underline for italic. */
	if (rval & RS_ULINE || unlikely(rval & RS_ITALIC)) {
		xcb_poly_line(jbxvt.X.xcb, XCB_COORD_MODE_ORIGIN,
			jbxvt.X.win.vt, jbxvt.X.gc.tx, 2,
			(xcb_point_t[]){{p.x, p.y},
			{p.x + len * jbxvt.X.font_size.width,
			p.y}});
		xcb_flush(jbxvt.X.xcb);
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

