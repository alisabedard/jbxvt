/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "repaint.h"
//#define DEBUG
#include "color.h"
#include "color_index.h"
#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "selection.h"
#include "show_selection.h"
#include "slinest.h"

#include <stdlib.h>
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
	CLOG("byte is 0x%x, r: 0x%x, g: 0x%x, b: 0x%x, pixel is 0x%x",
		c, r, g, b, p);
	return p;
}

// Set 9-bit color
static pixel_t set_rgb_colors(const uint16_t c, const bool fg)
{
	pixel_t p = get_pixel_for_word(c);
	set_color(fg?XCB_GC_FOREGROUND :XCB_GC_BACKGROUND, c, jbxvt.X.gc.tx);
	return p;
}

static pixel_t set_index_colors(const uint8_t index, const bool fg)
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
	const bool rvid = (rval & RS_RVID) || (rval & RS_BLINK);
	const bool bold = rval & RS_BOLD;
	bool cmod = set_rval_colors(rval);
	if (rvid) { // Reverse looked up colors.
		LOG("rvid");
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FOREGROUND
			| XCB_GC_BACKGROUND, (uint32_t[]){
			jbxvt.X.color.current_bg, jbxvt.X.color.current_fg});
		cmod = true;
	}
	p.y += jbxvt.X.font_ascent;
	if(bold) {
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FONT,
			&(uint32_t){jbxvt.X.bold_font});
	}
	// Draw text with background:
	xcb_image_text_8(jbxvt.X.xcb, len, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, p.x, p.y, (const char *)str);
	xcb_flush(jbxvt.X.xcb);
	++p.y; // Advance for underline, use underline for italic.
	if (rval & RS_ULINE || unlikely(rval & RS_ITALIC)) {
		xcb_poly_line(jbxvt.X.xcb, XCB_COORD_MODE_ORIGIN,
			jbxvt.X.win.vt, jbxvt.X.gc.tx, 2, (xcb_point_t[]){
			{p.x, p.y}, {p.x + len * jbxvt.X.font_width, p.y}});
		xcb_flush(jbxvt.X.xcb);
	}
	if(bold) { // restore font
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FONT,
			&(uint32_t){jbxvt.X.font});
	}
	if (cmod) {
		set_fg(NULL);
		set_bg(NULL);
	}
}

// Display the string using the rendition vector at the screen coordinates
static void paint_rvec_text(uint8_t * str,
	uint32_t * rvec, uint16_t len, xcb_point_t p)
{
	if (rvec == NULL) {
		paint_rval_text(str, 0, len, p);
		return;
	}
	while (len > 0) {
		uint_fast16_t i;
		// find the length for which the current rend val applies
		for (i = 0; i < len && rvec[i] == *rvec; ++i)
			  ;
		// draw
		paint_rval_text(str,rvec[0], i, p);
		// advance
		str += i;
		rvec += i;
		len -= i;
		p.x += i * jbxvt.X.font_width;
	}
}

static int_fast32_t repaint_generic(const xcb_point_t p,
	const int_fast32_t m, const int_fast32_t c1,
	const int_fast32_t c2, uint8_t * restrict str, uint32_t * rend)
{
	paint_rvec_text(str, rend ? rend + c1 : NULL, m, p);
	const int_fast16_t x = p.x + m * jbxvt.X.font_width;
	const uint_fast16_t width = (c2 - c1 + 1 - m)
		* jbxvt.X.font_width;
	if (width > 0)
		  xcb_clear_area(jbxvt.X.xcb, false, jbxvt.X.win.vt, x,
			  p.y, width, jbxvt.X.font_height);
	return p.y + jbxvt.X.font_height;
}

__attribute__((nonnull(3)))
static int_fast16_t show_scroll_history(xcb_point_t rc1, xcb_point_t rc2,
	xcb_point_t * restrict p, uint8_t * restrict str)
{
	int_fast16_t line = rc1.y;
	if (!str)
		  return line;
	for (int_fast32_t i = jbxvt.scr.offset - 1 - rc1.y;
		line <= rc2.y && i >= 0; ++line, --i) {
		struct slinest * sl = jbxvt.scr.sline.data[i];
		if(!sl || sl->canary) // prevent segfault
			  break;
		const uint_fast8_t l = sl->sl_length;
		const uint_fast8_t v = rc2.x + 1;
		const int32_t m = ((v < l) ? v : l) - rc1.x;
		if (m < 0)
			  return line;
		if (m >= jbxvt.scr.chars.width)
			  return line;
		// history chars already sanitized, so just use them:
		memcpy(str, sl->sl_text + rc1.x, m);
		p->y = repaint_generic(*p, m, rc1.x, rc2.x,
			str, sl->sl_rend);
	}
	return line;
}

/* Repaint the box delimited by rc1.y to rc2.y and rc1.x to rc2.x
   of the displayed screen from the backup screen.  */
void repaint(xcb_point_t rc1, xcb_point_t rc2)
{
#ifdef DEBUG_REPAINT
	LOG("repaint({%d, %d}, {%d, %d})", rc1.x, rc1.y, rc2.x, rc2.y);
#endif//DEBUG_REPAINT
	xcb_point_t p = { .x = MARGIN + rc1.x * jbxvt.X.font_width,
		.y = MARGIN + rc1.y * jbxvt.X.font_height};
	// Allocate enough space to process each column, plus wrap byte.
	uint8_t str[jbxvt.scr.chars.width + 1];
	//  First do any 'scrolled off' lines that are visible.
	int_fast32_t line = show_scroll_history(rc1, rc2, &p, str);

	// Do the remainder from the current screen:
	int_fast32_t i = jbxvt.scr.offset > rc1.y ? 0
		: rc1.y - jbxvt.scr.offset;

	for (; line <= rc2.y; ++line, ++i) {
		uint8_t * s = jbxvt.scr.current->text[i];
		register uint_fast8_t x;
		for (x = rc1.x; s && x <= rc2.x; x++)
			str[x - rc1.x] = s[x] < ' ' ? ' ' : s[x];
		const uint16_t m = x - rc1.x;
		p.y = repaint_generic(p, m, rc1.x, rc2.x, str,
			jbxvt.scr.current->rend[i]);
	}
	xcb_flush(jbxvt.X.xcb);
	show_selection(rc1.y,rc2.y,rc1.x,rc2.x);
}

