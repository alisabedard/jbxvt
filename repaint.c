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

static bool set_rval_colors(const uint32_t rval)
{
	bool fg_mod = true, bg_mod = true;
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
		  set_fg(NULL);
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
	} else
		  fg_mod = false;

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
		  set_bg(NULL);
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
	} else bg_mod = false;

	return fg_mod || bg_mod;
}

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, xcb_point_t p)
{
	const bool rvid = rval & RS_RVID || rval & RS_BLINK;
	bool cmod = set_rval_colors(rval);
	if (rvid) { // Reverse looked up colors.
		xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FOREGROUND
			| XCB_GC_BACKGROUND, (uint32_t[]){
			jbxvt.X.color.current_bg, jbxvt.X.color.current_fg});
		cmod = true;
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

	++p.y; // Advance for underline, use underline for italic.
	if (rval & RS_ULINE || rval & RS_ITALIC) {
		xcb_poly_line(jbxvt.X.xcb, XCB_COORD_MODE_ORIGIN,
			jbxvt.X.win.vt, jbxvt.X.gc.tx, 2, (xcb_point_t[]){
			{p.x, p.y}, {p.x + len * jbxvt.X.font_width, p.y}});
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

static int_fast16_t show_scroll_history(xcb_point_t rc1, xcb_point_t rc2,
	xcb_point_t * restrict p, uint8_t * restrict str)
{
	int_fast16_t line = rc1.y;
	for (int_fast32_t i = jbxvt.scr.offset - 1 - rc1.y;
		line <= rc2.y && i >= 0; ++line, --i) {
		struct slinest * sl = jbxvt.scr.sline.data[i];
		if(!sl) // prevent segfault
			  continue;
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
	uint8_t * str = malloc(jbxvt.scr.chars.width + 1);
	//  First do any 'scrolled off' lines that are visible.
	int_fast32_t line = show_scroll_history(rc1, rc2, &p, str);

	// Do the remainder from the current screen:
	int_fast32_t i = jbxvt.scr.offset > rc1.y ? 0
		: rc1.y - jbxvt.scr.offset;
	for (; line <= rc2.y; ++line, ++i) {
		uint8_t * s = jbxvt.scr.current->text[i];
		uint8_t m = rc2.x - rc1.x;
		memcpy(str - rc1.x, s, m);
		register int_fast16_t c;
		for(c = m; c && s[c] < ' '; --c);
			  ; // eliminate junk after '\0' and find length
		for(m = c; c >= 0; --c) // fix interior bad chars
			  if (s[c] < ' ')
				    str[c - rc1.x] = ' ';
		p.y = repaint_generic(p, m + 1, rc1.x, rc2.x, str,
			jbxvt.scr.current->rend[i]);
	}
	free(str);
	show_selection(rc1.y,rc2.y,rc1.x,rc2.x);
}

