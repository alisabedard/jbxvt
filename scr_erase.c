/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_erase.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"

#include <string.h>

static void zero_line(uint8_t * restrict s,
	uint32_t * restrict r, uint16_t sz)
{
	if (s)
		memset(s, 0, sz + 1); // +1 for wrap flag
	if (r)
		memset(r, 0, sz<<2);
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
	LOG("scr_erase_line(%d)", mode);
	home_screen();
	xcb_rectangle_t g = { .y = MARGIN + jbxvt.scr.current->cursor.y
			* jbxvt.X.font_height };
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->cursor.y];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->cursor.y];
	switch (mode) {
	case 1:
		LOG("START");
		get_horz_geo(&g, jbxvt.scr.current->cursor.x, 0);
		zero_line(s, r, jbxvt.scr.current->cursor.x);
		break;
	case 0:
		LOG("END");
		get_horz_geo(&g, jbxvt.scr.chars.width
			- jbxvt.scr.current->cursor.x,
			jbxvt.scr.current->cursor.x);
		zero_line(s + jbxvt.scr.current->cursor.x,
			r + jbxvt.scr.current->cursor.x,
			jbxvt.scr.chars.width - jbxvt.scr.current->cursor.x);
		break;
	case 2:
		LOG("ENTIRE");
		get_horz_geo(&g, jbxvt.scr.chars.width, 0);
		zero_line(s, r, jbxvt.scr.chars.width);
		break;
	}
	cursor(CURSOR_DRAW); //clear
	check_selection(jbxvt.scr.current->cursor.y,
		jbxvt.scr.current->cursor.y);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, g.x, g.y, g.width,
		jbxvt.X.font_height);
	jbxvt.scr.current->wrap_next = false;
	cursor(CURSOR_DRAW);
}

//  erase part or the whole of the screen
void scr_erase_screen(const int8_t mode)
{
	LOG("scr_erase_screen(%d)", mode);
	home_screen();
	jbxvt.scr.current->wrap_next = 0;
	uint16_t i, width = jbxvt.X.font_width * jbxvt.scr.chars.width;
	const uint16_t wsz = jbxvt.scr.chars.width + 1;
	int16_t x = MARGIN, y, height;
	switch (mode) {
	case 1:
		LOG("START");
		y = MARGIN;
		height = jbxvt.scr.current->cursor.y * jbxvt.X.font_height;
		for (i = 0; i < jbxvt.scr.current->cursor.y; i++) {
			memset(jbxvt.scr.current->text[i],0, wsz);
			memset(jbxvt.scr.current->rend[i],0,
				wsz * sizeof(int32_t));
		}
		check_selection(0,jbxvt.scr.current->cursor.y - 1);
		if (height > 0) {
			xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
				x, y, width, height);
		}
		scr_erase_line(mode);
		break;
	case 0:
		LOG("END");
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
			check_selection(jbxvt.scr.current->cursor.y + 1,
				jbxvt.scr.chars.height - 1);
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
	case 2:
		LOG("ENTIRE");
		y = MARGIN;
		height = jbxvt.scr.chars.height * jbxvt.X.font_height;
		scroll(0, jbxvt.scr.chars.height - 1, jbxvt.scr.chars.height);
		cursor(CURSOR_DRAW);
		xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, x, y,
			width, height);
		cursor(CURSOR_DRAW);
		sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
			0, jbxvt.scr.chars.height - 1);
		break;
	default :
		LOG("UNKNOWN");
		return;
	}
}

