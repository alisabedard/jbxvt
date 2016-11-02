/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "paint.h"
#include "color_index.h"
#include "font.h"
#include "double.h"
#include "handle_sgr.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "screen.h"
#include "window.h"
#include <string.h>
#define DEBUG_PAINT
#ifndef DEBUG_PAINT
#undef LOG
#define LOG(...)
#endif//!DEBUG_PAINT
static struct {
	pixel_t bg, fg, current_fg, current_bg;
} paint_data;
void jbxvt_init_colors(xcb_connection_t * xc)
{
	paint_data.fg = jbxvt_set_fg(xc, jbxvt.opt.fg);
	paint_data.bg = jbxvt_set_fg(xc, jbxvt.opt.bg);
}
void jbxvt_reverse_colors(xcb_connection_t * xc)
{
	JB_SWAP(pixel_t, paint_data.fg, paint_data.bg);
	JB_SWAP(pixel_t, paint_data.current_fg, paint_data.current_bg);
	xcb_change_gc(xc, jbxvt_get_text_gc(xc), XCB_GC_FOREGROUND
		| XCB_GC_BACKGROUND, (uint32_t[]){
		paint_data.fg, paint_data.bg});
	xcb_change_window_attributes(xc, jbxvt_get_vt_window(xc),
		XCB_CW_BACK_PIXEL, &paint_data.bg);
}
xcb_gcontext_t jbxvt_get_text_gc(xcb_connection_t * xc)
{
	static xcb_gcontext_t gc;
	if (gc)
		return gc;
	return gc = xcb_generate_id(xc);
}
// not pure, has side-effects
static pixel_t fg(xcb_connection_t * xc, const pixel_t p)
{
	return paint_data.current_fg
		= jb_set_fg(xc, jbxvt_get_text_gc(xc), p);
}
// not pure, has side-effects
static pixel_t bg(xcb_connection_t * xc, const pixel_t p)
{
	return paint_data.current_bg
		= jb_set_bg(xc, jbxvt_get_text_gc(xc), p);
}
static pixel_t set_x(xcb_connection_t * xc, const char * color,
	const pixel_t backup, pixel_t (*func)(xcb_connection_t *,
	const pixel_t))
{
	return func(xc, color ? jb_get_pixel(xc,
		jbxvt_get_colormap(xc), color) : backup);
}
pixel_t jbxvt_get_fg(void)
{
	return paint_data.fg;
}
pixel_t jbxvt_get_bg(void)
{
	return paint_data.bg;
}
pixel_t jbxvt_set_fg(xcb_connection_t * xc, const char * color)
{
	return set_x(xc, color, paint_data.fg, &fg);
}
pixel_t jbxvt_set_bg(xcb_connection_t * xc, const char * color)
{
	return set_x(xc, color, paint_data.bg, &bg);
}
// 9-bit color
static pixel_t rgb_pixel(xcb_connection_t * xc, const uint16_t c)
{
	// Mask and scale to 8 bits.
	uint16_t r = (c & 0700) >> 4, g = (c & 070) >> 2, b = c & 7;
	//const uint8_t o = 13; // scale, leave 3 bits for octal
	const uint8_t o = 12; // scale, leave 3 bits for octal
	// Convert from 3 bit to 16 bit:
	r <<= o; g <<= o; b <<= o;
	const pixel_t p = jb_get_rgb_pixel(xc,
		jbxvt_get_colormap(xc), r, g, b);
	LOG("byte is 0x%x, r: 0x%x, g: 0x%x, b: 0x%x,"
		" pixel is 0x%x", c, r, g, b, p);
	return p;
}
static bool set_rstyle_colors(xcb_connection_t * xc, const uint32_t rstyle)
{
	// Mask foreground colors, 9 bits offset by 6 bits
	// Mask background colors, 9 bits offset by 15 bits
	const uint8_t color[] = {rstyle >> 7, rstyle >> 16};
	const bool rgb[] = {rstyle & JBXVT_RS_FG_RGB, rstyle & JBXVT_RS_BG_RGB};
	const bool ind[] = {rstyle & JBXVT_RS_FG_INDEX, rstyle & JBXVT_RS_BG_INDEX};
	if (ind[0])
		fg(xc, jbxvt_color_index[color[0]]);
	else if (rgb[0])
		fg(xc, rgb_pixel(xc, color[0]));
	if (ind[1])
		bg(xc, jbxvt_color_index[color[1]]);
	else if (rgb[1])
		bg(xc, rgb_pixel(xc, color[1]));
	return rgb[0] || rgb[1] || ind[0] || ind[1];
}
static inline void font(xcb_connection_t * xc, const xcb_font_t f)
{
	xcb_change_gc(xc, jbxvt_get_text_gc(xc), XCB_GC_FONT, &f);
}
static void draw_underline(xcb_connection_t * xc, uint16_t len,
	struct JBDim p, int8_t offset)
{
	xcb_poly_line(xc, XCB_COORD_MODE_ORIGIN, jbxvt_get_vt_window(xc),
		jbxvt_get_text_gc(xc), 2,
		(struct xcb_point_t[]){{p.x, p.y + offset},
		{p.x + len * jbxvt_get_font_size().width, p.y + offset}});
}
static void draw_text(xcb_connection_t * xc,
	uint8_t * restrict str, uint16_t len,
	struct JBDim * restrict p, uint32_t rstyle)
{
	xcb_image_text_8(xc, len, jbxvt_get_vt_window(xc),
		jbxvt_get_text_gc(xc), p->x, p->y, (const char *)str);
	++p->y; // Padding for underline, use underline for italic
	if (((rstyle & JBXVT_RS_ITALIC)
		&& (jbxvt_get_italic_font(xc) == jbxvt_get_normal_font(xc)))
		|| (rstyle & JBXVT_RS_UNDERLINE))
		draw_underline(xc, len, *p, 0);
	if (rstyle & JBXVT_RS_DOUBLE_UNDERLINE) {
		draw_underline(xc, len, *p, -2);
		draw_underline(xc, len, *p, 0);
	}
	if (rstyle & JBXVT_RS_CROSSED_OUT)
		draw_underline(xc, len, *p, -(jbxvt_get_font_size().h>>1));
}
static void set_reverse_video(xcb_connection_t * xc)
{
	jb_set_fg(xc, jbxvt_get_text_gc(xc), paint_data.current_bg);
	jb_set_bg(xc, jbxvt_get_text_gc(xc), paint_data.current_fg);
}
//  Paint the text using the rendition value at the screen position.
void jbxvt_paint(xcb_connection_t * xc, uint8_t * restrict str,
	uint32_t rstyle, uint16_t len, struct JBDim p, const bool dwl)
{
	if (!str || len < 1) // prevent segfault
		  return;
	if (rstyle & JBXVT_RS_INVISIBLE)
		  return; // nothing to do
	const bool rvid = (rstyle & JBXVT_RS_RVID)
		|| (rstyle & JBXVT_RS_BLINK);
	bool cmod = set_rstyle_colors(xc, rstyle);
	if (rvid) { // Reverse looked up colors.
		set_reverse_video(xc);
		cmod = true;
	}
	p.y += jbxvt_get_font_ascent();
	if (rstyle & JBXVT_RS_BOLD)
		font(xc, jbxvt_get_bold_font(xc));
	if (rstyle & JBXVT_RS_ITALIC)
		font(xc, jbxvt_get_italic_font(xc));
	// Draw text with background:
	if (dwl)
		str = jbxvt_get_double_width_string(str, &len);
	draw_text(xc, str, len, &p, rstyle);
	if (dwl)
		free(str);
	if(rstyle & JBXVT_RS_BOLD || rstyle & JBXVT_RS_ITALIC)
		font(xc, jbxvt_get_normal_font(xc)); // restore font
	if (cmod) {
		fg(xc, paint_data.fg);
		bg(xc, paint_data.bg);
	}
}
