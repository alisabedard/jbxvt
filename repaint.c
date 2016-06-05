/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "repaint.h"

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

static void set_rval_colors(const uint32_t rval)
{
	// normal foregrounds:
	if (rval & RS_F0)
		  set_fg(COLOR_0);
	else if (rval & RS_F1)
		  set_fg(COLOR_1);
	else if (rval & RS_F2)
		  set_fg(COLOR_2);
	else if (rval & RS_F3)
		  set_fg(COLOR_3);
	else if (rval & RS_F4)
		  set_fg(COLOR_4);
	else if (rval & RS_F5)
		  set_fg(COLOR_5);
	else if (rval & RS_F6)
		  set_fg(COLOR_6);
	else if (rval & RS_F7)
		  set_fg(COLOR_7);
	else if (rval & RS_FR)
		  reset_fg();
	else if (rval & RS_BF) {
		// bright foregrounds:
		if (rval & RS_F0)
			  set_fg(BCOLOR_0);
		else if (rval & RS_F1)
			  set_fg(BCOLOR_1);
		else if (rval & RS_F2)
			  set_fg(BCOLOR_2);
		else if (rval & RS_F3)
			  set_fg(BCOLOR_3);
		else if (rval & RS_F4)
			  set_fg(BCOLOR_4);
		else if (rval & RS_F5)
			  set_fg(BCOLOR_5);
		else if (rval & RS_F6)
			  set_fg(BCOLOR_6);
		else if (rval & RS_F7)
			  set_fg(BCOLOR_7);
	}
	// normal backgrounds:
	if (rval & RS_B0)
		  set_bg(COLOR_0);
	else if (rval & RS_B1)
		  set_bg(COLOR_1);
	else if (rval & RS_B2)
		  set_bg(COLOR_2);
	else if (rval & RS_B3)
		  set_bg(COLOR_3);
	else if (rval & RS_B4)
		  set_bg(COLOR_4);
	else if (rval & RS_B5)
		  set_bg(COLOR_5);
	else if (rval & RS_B6)
		  set_bg(COLOR_6);
	else if (rval & RS_B7)
		  set_bg(COLOR_7);
	else if (rval & RS_BR)
		  reset_bg();
	else if (rval & RS_BB) {
		// bright backgrounds:
		if (rval & RS_B0)
			  set_bg(BCOLOR_0);
		else if (rval & RS_B1)
			  set_bg(BCOLOR_1);
		else if (rval & RS_B2)
			  set_bg(BCOLOR_2);
		else if (rval & RS_B3)
			  set_bg(BCOLOR_3);
		else if (rval & RS_B4)
			  set_bg(BCOLOR_4);
		else if (rval & RS_B5)
			  set_bg(BCOLOR_5);
		else if (rval & RS_B6)
			  set_bg(BCOLOR_6);
		else if (rval & RS_B7)
			  set_bg(BCOLOR_7);
	}
}

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, xcb_point_t p)
{
	set_rval_colors(rval);
	if (rval & RS_RVID || rval & RS_BLINK) { // Reverse looked up colors.
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FOREGROUND
			| XCB_GC_BACKGROUND, (uint32_t[]){
			jbxvt.X.color.current_bg, jbxvt.X.color.current_fg});
	}
	p.y+= jbxvt.X.font_ascent;

	// Draw text with background:
	xcb_image_text_8(jbxvt.X.xcb, len, jbxvt.X.win.vt, jbxvt.X.gc.tx,
		p.x, p.y, (const char *)str);

#if 0
	if (rval & RS_BOLD) { // Fake bold:
		// FIXME: no proper xcb equivalent
		XDrawString(jbxvt.X.dpy,jbxvt.X.win.vt,
			jbxvt.X.gc.tx, p.x + 1, p.y,
			(const char *)str,len);
	}
#endif

	p.y++; // Advance for underline, use underline for italic.
	if (rval & RS_ULINE || rval & RS_ITALIC) {
		xcb_poly_line(jbxvt.X.xcb, XCB_COORD_MODE_ORIGIN,
			jbxvt.X.win.vt, jbxvt.X.gc.tx, 2, (xcb_point_t[]){
			{p.x, p.y}, {p.x + len * jbxvt.X.font_width, p.y}});
	}
	reset_color();
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
		for (i = 0; i < len; ++i)
			if (rvec[i] != rvec[0])
				break;
		paint_rval_text(str,rvec[0], i, p);
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

__attribute__((const))
static inline uint_fast8_t convert_char(const uint_fast8_t c)
{
	return c < ' ' ? ' ' : c;
}

/* Repaint the box delimited by rc1.y to rc2.y and rc1.x to rc2.x
   of the displayed screen from the backup screen.  */
void repaint(xcb_point_t rc1, xcb_point_t rc2)
{
	LOG("repaint(%d, %d, %d, %d)", rc1.y, rc2.y, rc1.x, rc2.x);
	uint8_t * str = malloc(jbxvt.scr.chars.width + 1);
	int y = rc1.y;
	int x1 = MARGIN + rc1.x * jbxvt.X.font_width;
	int y1 = MARGIN + rc1.y * jbxvt.X.font_height;
	int i;
	LOG("y:%d, x1:%d, y1:%d, i1: %d, i2 %d\n",
		y, x1, y1, jbxvt.scr.offset - 1 - rc1.y,
		rc1.y - jbxvt.scr.offset);
	//  First do any 'scrolled off' lines that are visible.
	for (i = jbxvt.scr.offset - 1 - rc1.y;
		y <= rc2.y && i >= 0; y++, i--) {
		struct slinest * sl = jbxvt.scr.sline.data[i];
		if(!sl) continue; // prevent segfault
		uint16_t m = (rc2.x + 1) < sl->sl_length
			? (rc2.x + 1) : sl->sl_length;
		uint8_t * s = sl->sl_text;
		m -= rc1.x;
		for (uint16_t x = 0; x < m; x++)
			  str[x] = convert_char(s[x + rc1.x]);
		y1 = repaint_generic((xcb_point_t){.x=x1, .y=y1}, m, rc1.x,
			rc2.x, str, sl->sl_rend);
	}

	// Do the remainder from the current screen:
	i = jbxvt.scr.offset > rc1.y ? 0 : rc1.y - jbxvt.scr.offset;
	for (; y <= rc2.y; y++, i++) {
		uint8_t * s = jbxvt.scr.current->text[i];
		uint8_t x;
		for (x = rc1.x; s && x <= rc2.x; x++)
			str[x - rc1.x] = convert_char(s[x]);
		const uint16_t m = x - rc1.x - 1;
		y1 = repaint_generic((xcb_point_t){.x=x1, .y=y1}, m,
			rc1.x, rc2.x, str,
			jbxvt.scr.current->rend[i]);
	}
	free(str);
	show_selection(rc1.y,rc2.y,rc1.x,rc2.x);
}

