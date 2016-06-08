/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_erase.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

static void zero_line(uint8_t * restrict s,
	uint32_t * restrict r, uint16_t sz)
{
	++sz;
	memset(s, 0, sz);
	memset(r, 0, sz * sizeof(uint32_t));
}

static void get_horz_geo(xcb_rectangle_t * restrict h,
	const uint16_t sz, const uint16_t col)
{
	h->x = MARGIN + col * jbxvt.X.font_width;
	h->width = sz * jbxvt.X.font_width;
}

//  erase part or the whole of a line
void scr_erase_line(const int8_t mode)
{
	home_screen();
	xcb_rectangle_t g = { .y = MARGIN + jbxvt.scr.current->cursor.y
			* jbxvt.X.font_height
	};
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->cursor.y];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->cursor.y];
	switch (mode) {
	    case START :
		get_horz_geo(&g, jbxvt.scr.current->cursor.x, 0);
		zero_line(s, r, jbxvt.scr.current->cursor.x);
		break;
	    case END :
		get_horz_geo(&g, jbxvt.scr.chars.width
			- jbxvt.scr.current->cursor.x,
			jbxvt.scr.current->cursor.x);
		zero_line(s + jbxvt.scr.current->cursor.x,
			r + jbxvt.scr.current->cursor.x,
			jbxvt.scr.chars.width
			- jbxvt.scr.current->cursor.x);
		break;
	    case ENTIRE :
		get_horz_geo(&g, jbxvt.scr.chars.width, 0);
		zero_line(s, r, jbxvt.scr.chars.width);
		break;
	    default :
		return;
	}
	/*  patch in the final rendition flag if there is any non-zero
	 *  rendition.  */
	r[jbxvt.scr.chars.width] = 0;
	for (uint16_t i = 0; i < jbxvt.scr.chars.width; i++) {
		if (r[i] != 0) {
			r[jbxvt.scr.chars.width] = 1;
			break;
		}
	}
	cursor(CURSOR_DRAW); //clear
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, g.x, g.y, g.width,
		jbxvt.X.font_height);
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

//  erase part or the whole of the screen
void scr_erase_screen(const int8_t mode)
{
	home_screen();
	jbxvt.scr.current->wrap_next = 0;
	uint16_t i, width = jbxvt.X.font_width * jbxvt.scr.chars.width;
	const uint16_t wsz = jbxvt.scr.chars.width + 1;
	int16_t x = MARGIN, y, height;
	switch (mode) {
	    case START :
		y = MARGIN;
		height = jbxvt.scr.current->cursor.y * jbxvt.X.font_height;
		for (i = 0; i < jbxvt.scr.current->cursor.y; i++) {
			memset(jbxvt.scr.current->text[i],0, wsz);
			memset(jbxvt.scr.current->rend[i],0,
				wsz * sizeof(int32_t));
		}
		if (height > 0) {
			xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
				x, y, width, height);
		}
		scr_erase_line(mode);
		break;
	    case END :
		if (jbxvt.scr.current->cursor.y
			|| jbxvt.scr.current->cursor.x) {
			y = MARGIN + (jbxvt.scr.current->cursor.y + 1)
				* jbxvt.X.font_height;
			height = (jbxvt.scr.chars.height
				- jbxvt.scr.current->cursor.y - 1)
				* jbxvt.X.font_height;
			for (i = jbxvt.scr.current->cursor.y + 1;
				i < jbxvt.scr.chars.height; ++i) {
				memset(jbxvt.scr.current->text[i],0, wsz);
				memset(jbxvt.scr.current->rend[i],0,
					wsz * sizeof(uint32_t));
			}
			if (height > 0) {
				xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
					x, y, width, height);
			}
			scr_erase_line(mode);
			break;
		}
		/*  If we are positioned at the top left hand corner then
		 *  it is effectively a whole screen clear.
		 *  Drop through so that we do not need to duplicate
		 *  the scroll-up code.  */
	    case ENTIRE :
		y = MARGIN;
		height = jbxvt.scr.chars.height * jbxvt.X.font_height;
		cursor(CURSOR_DRAW);
		xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, x, y,
			width, height);
		cursor(CURSOR_DRAW);
		sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
			0, jbxvt.scr.chars.height - 1);
	}
}

