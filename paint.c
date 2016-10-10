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

pixel_t jbxvt_set_fg(const char * color)
{
	return set_x(color, jbxvt.X.color.fg, &fg);
}

pixel_t jbxvt_set_bg(const char * color)
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

static bool set_rstyle_colors(const uint32_t rstyle)
{
	// Mask foreground colors, 9 bits offset by 6 bits
	// Mask background colors, 9 bits offset by 15 bits
	const uint8_t color[] = {rstyle >> 7, rstyle >> 16};
	const bool rgb[] = {rstyle & JBXVT_RS_FG_RGB, rstyle & JBXVT_RS_BG_RGB};
	const bool ind[] = {rstyle & JBXVT_RS_FG_INDEX, rstyle & JBXVT_RS_BG_INDEX};
	if (ind[0])
		fg(color_index[color[0]]);
	else if (rgb[0])
		fg(rgb_pixel(color[0]));
	if (ind[1])
		bg(color_index[color[1]]);
	else if (rgb[1])
		bg(rgb_pixel(color[1]));
	return rgb[0] || rgb[1] || ind[0] || ind[1];
}

static inline void font(const xcb_font_t f)
{
	xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FONT, &f);
}

static void draw_underline(uint16_t len, struct JBDim p)
{
	xcb_poly_line(jbxvt.X.xcb, XCB_COORD_MODE_ORIGIN, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, 2,
		(struct xcb_point_t[]){{p.x, p.y},
		{p.x + len * jbxvt.X.font.size.width, p.y}});
}

static void draw_text(uint8_t * restrict str, uint16_t len,
	struct JBDim * restrict p, uint32_t rstyle)
{
	xcb_image_text_8(jbxvt.X.xcb, len, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, p->x, p->y, (const char *)str);
	++p->y; // Padding for underline, use underline for italic
	if (rstyle & JBXVT_RS_ULINE || unlikely(rstyle & JBXVT_RS_ITALIC))
		draw_underline(len, *p);
}

//  Paint the text using the rendition value at the screen position.
void paint_rstyle_text(uint8_t * restrict str, uint32_t rstyle,
	int16_t len, struct JBDim p)
{
	if (!str || len < 1) // prevent segfault
		  return;
	if (rstyle & JBXVT_RS_INVISIBLE)
		  return; // nothing to do
	const bool rvid = (rstyle & JBXVT_RS_RVID) || (rstyle & JBXVT_RS_BLINK);
	const bool bold = rstyle & JBXVT_RS_BOLD;
	bool cmod = set_rstyle_colors(rstyle);
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
#ifdef JBXVT_FIXME
	if (jbxvt.mode.decdwl || (rstyle & JBXVT_RS_DWL)) {
		LOG("PAINTING DWL");
		for (int16_t i = 0; i < len; ++i) {
			const char buf[2] = {str[i], ' '};
			p.x += jbxvt.X.f.size.w * 2;
			xcb_image_text_8(c, 2, jbxvt.X.win.vt,
				gc, p.x, p.y, buf);
		}
		p.y += jbxvt.X.f.size.h;
	} else
#endif//JBXVT_FIXME
	draw_text(str, len, &p, rstyle);
	if(bold) // restore font
		font(jbxvt.X.f.normal);
	if (cmod) {
		fg(jbxvt.X.color.fg);
		bg(jbxvt.X.color.bg);
	}
}

