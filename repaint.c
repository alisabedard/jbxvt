/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "repaint.h"
//#define DEBUG
#include "color.h"
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

struct ColorFlag {
	uint32_t flag;
	char * color;
};

static uint8_t color_index[256] = {
	// 0 is 0
	[1] = 0x20, [2] = 0x8, [3] = 0x28, [4] = 0x2, [5] = 0x22, [6] = 0xa,
	[7] = 0x2a, [8] = 0x3f, [9] = 0x30,

	[10] = 0xc, [11] = 0x3c, [12] = 0x3, [13] = 0x33, [14] = 0xf, [15] = 0x3f,
	// 16 is 0
	[17] = 0x1, [18] = 0x2, [19] = 0x2,

	[20] = 0x3, [21] = 0x3, [22] = 0x4, [23] = 0x5, [24] = 0x5, [25] = 0x6,
	[26] = 0x7, [27] = 0x7, [28] = 0x8, [29] = 0x9,

	[30] = 0xa, [31] = 0xb, [32] = 0x9, [33] = 0xa, [34] = 0xb,
	[35] = 0x9, [36] = 0xa, [37] = 0xa, [38] = 0xb, [39] = 0xb,

	[40] = 0xe, [41] = 0xd, [42] = 0xe, [43] = 0xe,
	[44] = 0xf, [45] = 0xf, [46] = 0xc, [47] = 0xd,

	[64] = 0x28,
	[81] = 0x2f,
	[121] = 0x2e,
	[159] = 0x2f,
	[225] = 0x3b
};

static struct ColorFlag color_flags [] = {
	{RS_F0, COLOR_0},
	{RS_F1, COLOR_1},
	{RS_F2, COLOR_2},
	{RS_F3, COLOR_3},
	{RS_F4, COLOR_4},
	{RS_F5, COLOR_5},
	{RS_F6, COLOR_6},
	{RS_F7, COLOR_7},
	{RS_BF|RS_F0, BCOLOR_0},
	{RS_BF|RS_F1, BCOLOR_1},
	{RS_BF|RS_F2, BCOLOR_2},
	{RS_BF|RS_F3, BCOLOR_3},
	{RS_BF|RS_F4, BCOLOR_4},
	{RS_BF|RS_F5, BCOLOR_5},
	{RS_BF|RS_F6, BCOLOR_6},
	{RS_BF|RS_F7, BCOLOR_7},
	{RS_B0, COLOR_0},
	{RS_B1, COLOR_1},
	{RS_B2, COLOR_2},
	{RS_B3, COLOR_3},
	{RS_B4, COLOR_4},
	{RS_B5, COLOR_5},
	{RS_B6, COLOR_6},
	{RS_B7, COLOR_7},
	{RS_BB|RS_B0, BCOLOR_0},
	{RS_BB|RS_B1, BCOLOR_1},
	{RS_BB|RS_B2, BCOLOR_2},
	{RS_BB|RS_B3, BCOLOR_3},
	{RS_BB|RS_B4, BCOLOR_4},
	{RS_BB|RS_B5, BCOLOR_5},
	{RS_BB|RS_B6, BCOLOR_6},
	{RS_BB|RS_B7, BCOLOR_7},
	{0} // terminator
};

static pixel_t get_pixel_for_byte(const uint8_t c)
{
	// Mask and scale to 8 bits.
	const uint8_t r = (c & 0x30) << 2;
	const uint8_t g = (c & 0xc) << 4;
	const uint8_t b = (c & 0x3) << 6;
	pixel_t p = get_pixel_rgb(r, g, b);
	LOG("byte is 0x%x, r: 0x%x, g: 0x%x, b: 0x%x, pixel is 0x%x",
		c, r, g, b, p);
	return p;
}

// Set 6-bit color
#define COLOR_IS_FG 0x80
static pixel_t set_rgb_colors(const uint8_t c)
{
	pixel_t p = get_pixel_for_byte(c&~COLOR_IS_FG);
	set_color((c & COLOR_IS_FG)?XCB_GC_FOREGROUND
		:XCB_GC_BACKGROUND, p, jbxvt.X.gc.tx);
	return p;
}

static pixel_t set_index_colors(const uint8_t index, const bool fg)
{
	LOG("set_index_colors, index: %d", index);
	pixel_t p = get_pixel_for_byte(color_index[index]);
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
	// Mask foreground colors, 8 bits offset by 6 bits
	uint32_t f = rval & 0x1fc0;
	// Mask background colors, 8 bits offset by 14 bits
	uint32_t b = rval & 0x1fc000;
	bool fg_set = false, bg_set = false;

	if (fg_rgb_mode) {
		LOG("fg_rgb_mode");
		set_rgb_colors(f&COLOR_IS_FG);
		fg_set = true;
	} else if (fg_index_mode) {
		LOG("fg_index_mode");
		set_index_colors(f, true);
		fg_set = true;
	}

	if (bg_rgb_mode) {
		LOG("bg_rgb_mode");
		set_rgb_colors(b);
		bg_set = true;
	} else if (bg_index_mode) {
		LOG("bg_index_mode");
		set_index_colors(b, false);
		bg_set = true;
	}


	for(int_fast8_t i = 0; color_flags[i].flag; ++i) {
		if (!fg_set && color_flags[i].flag == f) {
			set_fg(color_flags[i].color);
			fg_set = true;
		}
		if (!bg_set && color_flags[i].flag == b) {
			set_bg(color_flags[i].color);
			bg_set = true;
		}
		if (bg_set && fg_set)
			  break;
	}
	return fg_set || bg_set;
}

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, xcb_point_t p)
{
	const bool rvid = rval & RS_RVID || rval & RS_BLINK;
	const bool bold = rval & RS_BOLD;
	bool cmod = set_rval_colors(rval);
	if (rvid) { // Reverse looked up colors.
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
	const int_fast32_t m, const int_fast32_t c1, const int_fast32_t c2,
	uint8_t * restrict str, uint32_t * rend)
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

static int_fast16_t show_scroll_history(xcb_point_t rc1, xcb_point_t rc2,
	xcb_point_t * restrict p, uint8_t * restrict str)
{
	int_fast16_t line = rc1.y;
	for (int_fast32_t i = jbxvt.scr.offset - 1 - rc1.y;
		line <= rc2.y && i >= 0; ++line, --i) {
		struct slinest * sl = jbxvt.scr.sline.data[i];
		if(!sl) // prevent segfault
			  break;
		const uint_fast8_t l = sl->sl_length;
		const uint_fast8_t v = rc2.x + 1;
		const uint_fast16_t m = (v < l ? v : l) - rc1.x;
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
	LOG("repaint({%d, %d}, {%d, %d})", rc1.x, rc1.y, rc2.x, rc2.y);
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

