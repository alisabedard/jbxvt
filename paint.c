/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "paint.h"

#include "color_index.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "screen.h"

#include <string.h>

//#define DEBUG_PAINT
#ifndef DEBUG_PAINT
#undef LOG
#define LOG(...)
#endif//!DEBUG_PAINT

// not pure, has side-effects
static pixel_t fg(const pixel_t p)
{
	return jbxvt.X.color.current_fg
		= jb_set_fg(jbxvt.X.xcb, jbxvt.X.gc.tx, p);
}

// not pure, has side-effects
static pixel_t bg(const pixel_t p)
{
	return jbxvt.X.color.current_bg
		= jb_set_bg(jbxvt.X.xcb, jbxvt.X.gc.tx, p);
}

static pixel_t set_x(const char * color, const pixel_t backup,
	pixel_t (*func)(const pixel_t))
{
	return func(color ? jb_get_pixel(jbxvt.X.xcb,
		jbxvt.X.screen->default_colormap, color) : backup);
}

pixel_t set_fg(const char * color)
{
	return set_x(color, jbxvt.X.color.fg, &fg);
}

pixel_t set_bg(const char * color)
{
	return set_x(color, jbxvt.X.color.bg, &bg);
}

// 9-bit color
static pixel_t rgb_pixel(const uint16_t c)
{
	// Mask and scale to 8 bits.
	uint16_t r = (c & 0700) >> 4, g = (c & 070) >> 2, b = c & 7;
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
	// Mask foreground colors, 9 bits offset by 6 bits
	// Mask background colors, 9 bits offset by 15 bits
	const uint8_t color[] = {rval >> 7, rval >> 16};
	const bool rgb[] = {rval & RS_FG_RGB, rval & RS_BG_RGB};
	const bool ind[] = {rval & RS_FG_INDEX, rval & RS_BG_INDEX};
	if (rgb[0] || ind[0])
		fg(rgb[0] ? rgb_pixel(color[0]) : color_index[color[0]]);
	if (rgb[1] || ind[1])
		bg(rgb[1] ? rgb_pixel(color[1]) : color_index[color[1]]);
	return rgb[0] || rgb[1] || ind[0] || ind[1];
}

static inline void font(const xcb_font_t f)
{
	xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FONT, &f);
}

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	int16_t len, struct JBDim p)
{
	if (!str || len < 1) // prevent segfault
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
	const struct JBDim f = jbxvt.X.f.size;
	if (jbxvt.mode.decdwl) {
		for (int16_t i = 0; i < len; ++i) {
			const char buf[2] = {str[i], ' '};
			xcb_image_text_8(c, 2, w, gc, p.x, p.y, buf);
			p.x += f.w << 1;
		}
		jbxvt.mode.decdwl = false;
	} else {
		xcb_image_text_8(c, len, w, gc, p.x, p.y, (const char *)str);
		++p.y; // Padding for underline, use underline for italic
		if (rval & RS_ULINE || unlikely(rval & RS_ITALIC)) {
			xcb_poly_line(c, XCB_COORD_MODE_ORIGIN, w, gc, 2,
				(struct xcb_point_t[]){{p.x, p.y},
				{p.x + len * f.w, p.y}});
		}
	}
	if(bold) // restore font
		font(jbxvt.X.f.normal);
	if (cmod) {
		fg(jbxvt.X.color.fg);
		bg(jbxvt.X.color.bg);
	}
}

