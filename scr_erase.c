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
	memset(s, 0, sz + 1); // +1 for wrap flag
	memset(r, 0, sz<<2);
}

static void get_horz_geo(xcb_rectangle_t * restrict h,
	const uint16_t sz, const uint16_t col)
{
	h->x = MARGIN + col * jbxvt.X.font_size.width;
	h->width = sz * jbxvt.X.font_size.width;
}

//  erase part or the whole of a line
void scr_erase_line(const int8_t mode)
{
	LOG("scr_erase_line(%d)", mode);
	home_screen();
	VTScreen * scr = jbxvt.scr.current;
	xcb_point_t c = scr->cursor;
	const uint8_t fh = jbxvt.X.font_size.height;
	xcb_rectangle_t g = { .y = MARGIN + c.y * fh };
	uint8_t * s = scr->text[c.y];
	uint32_t * r = scr->rend[c.y];
	const uint8_t cw = jbxvt.scr.chars.width;
	switch (mode) {
	case 1:
		LOG("START");
		get_horz_geo(&g, c.x, 0);
		zero_line(s, r, c.x);
		break;
	case 0:
		LOG("END");
		get_horz_geo(&g, cw - c.x, c.x);
		zero_line(s + c.x, r + c.x, cw - c.x);
		break;
	case 2:
		LOG("ENTIRE");
		get_horz_geo(&g, cw, 0);
		zero_line(s, r, cw);
		break;
	}
	cursor(CURSOR_DRAW); //clear
	check_selection(c.y, c.y);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, g.x, g.y, g.width, fh);
	scr->wrap_next = false;
	cursor(CURSOR_DRAW);
}

static void zero(const int_fast16_t i)
{
	const size_t sz = jbxvt.scr.chars.width;
	VTScreen * restrict s = jbxvt.scr.current;
	memset(s->text[i],0, sz + 1);
	memset(s->rend[i],0, sz << 2);
}

static void common_scr_erase(const xcb_rectangle_t r,
	const int16_t row1, const int16_t row2, const int8_t mode)
{
	check_selection(row1, row2);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, r.x, r.y,
		r.width, r.height);
	scr_erase_line(mode);
}

//  erase part or the whole of the screen
void scr_erase_screen(const int8_t mode)
{
	LOG("scr_erase_screen(%d)", mode);
	home_screen();
	VTScreen * s = jbxvt.scr.current;
	s->wrap_next = 0;
	const Size c = jbxvt.scr.chars;
	const uint8_t fh = jbxvt.X.font_size.height;
	const Size p = jbxvt.scr.pixels;
	xcb_rectangle_t r = {.x = MARGIN, .y = MARGIN,
		.width = p.width};
	switch (mode) {
	case 1:
		LOG("START");
		r.height = s->cursor.y * fh;
		for (uint8_t i = 0; i < s->cursor.y; ++i)
			  zero(i);
		common_scr_erase(r, 0, s->cursor.y - 1, mode);
		break;
	case 0:
		LOG("END");
		if (s->cursor.y || s->cursor.x) {
			r.y += (s->cursor.y + 1) * fh;
			r.height = (c.height - s->cursor.y - 1) * fh;
			for (uint8_t i = s->cursor.y + 1; i < c.height; ++i)
				  zero(i);
			common_scr_erase(r, s->cursor.y + 1, c.height - 1,
				mode);
			break;
		}
		/*  If we are positioned at the top left hand corner then
		 *  it is effectively a whole screen clear.
		 *  Drop through so that we do not need to duplicate
		 *  the scroll-up code.  */
	case 2:
		LOG("ENTIRE");
		if (!r.height) // test in case above case fell through
			r.height = c.h - 1;
		scroll(0, r.height, r.height);
		cursor(CURSOR_DRAW);
	}
}

