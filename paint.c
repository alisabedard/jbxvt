/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
//#undef DEBUG
#include "paint.h"
#include <stdlib.h>
#include "JBXVTPaintContext.h"
#include "color.h"
#include "color_index.h"
#include "double.h"
#include "font.h"
#include "gc.h"
#include "libjb/log.h"
#include "window.h"
#include "xcb_screen.h"
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
static void handle_underline_styles(struct JBXVTPaintContext * c)
{
	xcb_connection_t * xc = c->xc;
	const uint16_t len = c->length;
	const rstyle_t rstyle = *c->style;
	if (((rstyle & JBXVT_RS_ITALIC)
		&& (jbxvt_get_italic_font(xc)
		== jbxvt_get_normal_font(xc)))
		|| (rstyle & JBXVT_RS_UNDERLINE))
		draw_underline(xc, len, c->position, 0);
	if (rstyle & JBXVT_RS_DOUBLE_UNDERLINE) {
		draw_underline(xc, len, c->position, -2);
		draw_underline(xc, len, c->position, 0);
	}
	/* Test against JBXVT_RS_BG_RGB here to prevent red background
	   text from being rendered crossed out.  */
	if (rstyle & JBXVT_RS_CROSSED_OUT && !(rstyle & JBXVT_RS_BG_RGB))
		draw_underline(xc, len, c->position,
			-(jbxvt_get_font_size().h >> 1));
}
static void draw_text(xcb_connection_t * xc,
	uint8_t * restrict str, uint16_t len,
	struct JBDim * restrict p, rstyle_t rstyle)
{
	{ // vt, gc scope
		/* Cache frequently used values
		   to avoid function call overhead. */
		static xcb_window_t vt;
		static xcb_gcontext_t gc;
		if (!vt) {
			vt = jbxvt_get_vt_window(xc);
			gc = jbxvt_get_text_gc(xc);
		}
		// Draw the text:
		xcb_image_text_8(xc, len, vt, gc, p->x, p->y,
			(const char *)str);
	}
	++p->y; // Padding for underline, use underline for italic
	handle_underline_styles(&(struct JBXVTPaintContext){.xc = xc,
		.position = *p, .length = len, .style = &rstyle});
}
__attribute__((const))
static uint16_t get_channel(const uint16_t compressed_rgb,
	const uint16_t mask, const uint8_t shift)
{
	enum { SCALE = 12 }; // scale, leave 3 bits for octal
	return ((compressed_rgb & mask) >> shift) << SCALE;
}
// 9-bit color
static pixel_t rgb_pixel(xcb_connection_t * xc, const uint16_t c)
{
	enum {R_MASK = 0700, G_MASK = 070, B_MASK = 07, R_SHIFT = 4,
		G_SHIFT = 2, B_SHIFT = 0};
	// Mask and scale to 8 bits, then convert from 3 bit to 16 bit:
	const uint16_t r = get_channel(c, R_MASK, R_SHIFT), g = get_channel(c,
		G_MASK, G_SHIFT), b = get_channel(c, B_MASK, B_SHIFT);
	const pixel_t p = jb_get_rgb_pixel(xc,
		jbxvt_get_colormap(xc), r, g, b);
	LOG("byte is 0x%x, r: 0x%x, g: 0x%x, b: 0x%x,"
		" pixel is 0x%x", c, r, g, b, p);
	return p;
}
static void rstyle_color(xcb_connection_t * restrict xc, const uint8_t *
	restrict color, const bool * restrict ind, const bool * restrict rgb,
	pixel_t (*set_pixel)(xcb_connection_t *, const pixel_t),
	const uint8_t i)
{
	if (ind[i])
		set_pixel(xc, jbxvt_color_index[color[i]]);
	else if (rgb[i])
		set_pixel(xc, rgb_pixel(xc, color[i]));
}
static bool set_rstyle_colors(xcb_connection_t * restrict xc,
	const uint32_t rstyle)
{
	// Mask foreground colors, 9 bits offset by 7 bits
	// Mask background colors, 9 bits offset by 16 bits
	enum { FG_SHIFT = 7, BG_SHIFT = 16 };
	const uint8_t color[] = {rstyle >> FG_SHIFT, rstyle >> BG_SHIFT};
	const bool rgb[] = {rstyle & JBXVT_RS_FG_RGB,
		rstyle & JBXVT_RS_BG_RGB};
	const bool ind[] = {rstyle & JBXVT_RS_FG_INDEX,
		rstyle & JBXVT_RS_BG_INDEX};
	rstyle_color(xc, color, ind, rgb, jbxvt_set_fg_pixel, 0);
	rstyle_color(xc, color, ind, rgb, jbxvt_set_bg_pixel, 1);
	return rgb[0] || rgb[1] || ind[0] || ind[1];
}
static void restore_colors(xcb_connection_t * restrict xc)
{
	jbxvt_set_fg_pixel(xc, jbxvt_get_fg());
	jbxvt_set_bg_pixel(xc, jbxvt_get_bg());
}
static bool is_reverse_video(const rstyle_t rs)
{
	return (rs & JBXVT_RS_RVID) || (rs & JBXVT_RS_BLINK);
}
void jbxvt_paint(struct JBXVTPaintContext * restrict c)
{
	/* Storage of p instead of using c->position directly fixes scroll
	 * history display.  */
	struct JBDim p = c->position;
	const rstyle_t rstyle = *c->style;
	// Check if there is nothing to render:
	if (!c->string || c->length < 1 || rstyle & JBXVT_RS_INVISIBLE)
		  return;
	xcb_connection_t * xc = c->xc;
	bool cmod = set_rstyle_colors(xc, rstyle);
	const bool rvid = is_reverse_video(rstyle);
	if (rvid) { // Reverse looked up colors.
		jbxvt_set_reverse_video(xc);
		cmod = true;
	}
	p.y += jbxvt_get_font_ascent();
	bool font_mod = false;
	if (rstyle & JBXVT_RS_BOLD) {
		font(xc, jbxvt_get_bold_font(xc));
		font_mod = true;
	}
	if (rstyle & JBXVT_RS_ITALIC) {
		font(xc, jbxvt_get_italic_font(xc));
		font_mod = true;
	}
	// Draw text with background:
	if (c->is_double_width_line) // remember to free returned string:
		c->string = jbxvt_get_double_width_string(c->string,
			&c->length);
	if (c->string) { // Check that we are operating on valid memory.
		draw_text(xc, c->string, c->length, &p, rstyle);
		if (c->is_double_width_line)
			free(c->string);
	}
	if (font_mod)
		font(xc, jbxvt_get_normal_font(xc)); // restore font
	if (cmod)
		restore_colors(xc);
}
