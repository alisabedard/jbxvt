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
	uint8_t len, Dim p)
{
	set_rval_colors(rval);
	XGCValues v;
	XGetGCValues(jbxvt.X.dpy, jbxvt.X.gc.tx,
		GCForeground|GCBackground, &v);
	if (rval & RS_RVID || rval & RS_BLINK) { // Reverse looked up colors.
		XSetForeground(jbxvt.X.dpy, jbxvt.X.gc.tx, v.background);
		XSetBackground(jbxvt.X.dpy, jbxvt.X.gc.tx, v.foreground);
	}
	p.y+= jbxvt.X.font->ascent;

	// Draw text with background:
	XDrawImageString(jbxvt.X.dpy,jbxvt.X.win.vt,
		jbxvt.X.gc.tx, p.x, p.y,
		(const char *)str,len);
	if (rval & RS_BOLD) // Fake bold:
		  XDrawString(jbxvt.X.dpy,jbxvt.X.win.vt,
			  jbxvt.X.gc.tx, p.x + 1, p.y,
			  (const char *)str,len);
	p.y++; // Advance for underline, use underline for italic.
	if (rval & RS_ULINE || rval & RS_ITALIC)
		XDrawLine(jbxvt.X.dpy, jbxvt.X.win.vt, jbxvt.X.gc.tx,
			p.x, p.y, p.x + len * jbxvt.X.font_width, p.y);
	reset_color();
}

// Display the string using the rendition vector at the screen coordinates
static void paint_rvec_text(uint8_t * str,
	uint32_t * rvec, uint16_t len, Dim p)
{
	if (rvec == NULL) {
		paint_rval_text(str, 0, len, p);
		return;
	}
	while (len > 0) {
		uint16_t i;
		for (i = 0; i < len; i++)
			if (rvec[i] != rvec[0])
				break;
		paint_rval_text(str,rvec[0], i, p);
		str += i;
		rvec += i;
		len -= i;
		p.x += i * jbxvt.X.font_width;
	}
}

static int repaint_generic(const Dim p,
	const int m, const int c1, const int c2,
	uint8_t * restrict str, uint32_t * rend)
{
	paint_rvec_text(str, rend ? rend + c1 : NULL, m, p);
	const int x = p.x + m * jbxvt.X.font_width;
	const unsigned int width = (c2 - c1 + 1 - m)
		* jbxvt.X.font_width;
	if (width > 0)
		  XClearArea(jbxvt.X.dpy, jbxvt.X.win.vt, x, p.y,
			  width, jbxvt.X.font_height, false);
	return p.y + jbxvt.X.font_height;
}

__attribute__((const))
static uint8_t convert_char(const uint8_t c)
{
	return c < ' ' ? ' ' : c;
}

/* Repaint the box delimited by rc1.r to rc2.r and rc1.c to rc2.c
   of the displayed screen from the backup screen.  */
void repaint(Dim rc1, Dim rc2)
{
	LOG("repaint(%d, %d, %d, %d)", rc1.r, rc2.r, rc1.c, rc2.c);
	uint8_t * str = malloc(jbxvt.scr.chars.width + 1);
	int y = rc1.r;
	int x1 = MARGIN + rc1.c * jbxvt.X.font_width;
	int y1 = MARGIN + rc1.r * jbxvt.X.font_height;
	int i;
	LOG("y:%d, x1:%d, y1:%d, i1: %d, i2 %d\n",
		y, x1, y1, jbxvt.scr.offset - 1 - rc1.r,
		rc1.r - jbxvt.scr.offset);
	//  First do any 'scrolled off' lines that are visible.
	for (i = jbxvt.scr.offset - 1 - rc1.r;
		y <= rc2.r && i >= 0; y++, i--) {
		struct slinest * sl = jbxvt.scr.sline.data[i];
		if(!sl) continue; // prevent segfault
		uint16_t m = (rc2.c + 1) < sl->sl_length
			? (rc2.c + 1) : sl->sl_length;
		uint8_t * s = sl->sl_text;
		m -= rc1.c;
		for (uint16_t x = 0; x < m; x++)
			  str[x] = convert_char(s[x + rc1.c]);
		y1 = repaint_generic((Dim){.x=x1, .y=y1}, m, rc1.c,
			rc2.c, str, sl->sl_rend);
	}

	// Do the remainder from the current screen:
	i = jbxvt.scr.offset > rc1.r ? 0 : rc1.r - jbxvt.scr.offset;
	for (; y <= rc2.r; y++, i++) {
		uint8_t * s = jbxvt.scr.current->text[i];
		uint8_t x;
		for (x = rc1.c; x <= rc2.c; x++)
			  str[x - rc1.c] = convert_char(s[x]);
		const uint16_t m = x - rc1.c;
		y1 = repaint_generic((Dim){.x=x1, .y=y1}, m,
			rc1.c, rc2.c, str,
			jbxvt.scr.current->rend[i]);
	}
	free(str);
	show_selection(rc1.r,rc2.r,rc1.c,rc2.c);
}


