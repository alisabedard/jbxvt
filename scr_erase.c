/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_erase.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"

#include <string.h>

static void zero(const int16_t line, const uint16_t sz, const int16_t col)
{
	VTScreen * s = jbxvt.scr.current;
	memset(s->text[line] + col, 0, sz);
	memset(s->rend[line] + col, 0, sz << 2);
	s->wrap[line] = false;
}

static void get_horz_geo(xcb_rectangle_t * restrict h,
	const uint16_t sz, const uint16_t col)
{
	h->x = MARGIN + col * jbxvt.X.f.size.width;
	h->width = sz * jbxvt.X.f.size.width;
}

//  erase part or the whole of a line
void scr_erase_line(const int8_t mode)
{
	LOG("scr_erase_line(%d)", mode);
	change_offset(0);
	VTScreen * scr = jbxvt.scr.current;
	xcb_point_t c = scr->cursor;
	const uint8_t fh = jbxvt.X.f.size.height;
	xcb_rectangle_t g = { .y = MARGIN + c.y * fh };
	const uint8_t cw = jbxvt.scr.chars.width;
	switch (mode) {
	case 1:
		LOG("START");
		get_horz_geo(&g, c.x, 0);
		zero(c.y, c.x, 0);
		break;
	case 0:
		LOG("END");
		get_horz_geo(&g, cw - c.x, c.x);
		zero(c.y, cw - c.x, c.x);
		break;
	case 2:
		LOG("ENTIRE");
		get_horz_geo(&g, cw, 0);
		zero(c.y, cw, 0);
		break;
	}
	draw_cursor(); //clear
	check_selection(c.y, c.y);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
		g.x, g.y, g.width, fh);
	scr->wrap_next = false;
	draw_cursor();
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
	change_offset(0);
	VTScreen * s = jbxvt.scr.current;
	s->wrap_next = 0;
	const Size c = jbxvt.scr.chars;
	const uint8_t fh = jbxvt.X.f.size.height;
	const Size p = jbxvt.scr.pixels;
	xcb_rectangle_t r = {.x = MARGIN, .y = MARGIN,
		.width = p.width};
	const xcb_point_t cur = s->cursor;
	switch (mode) {
	case 1:
		LOG("START");
		r.height = cur.y * fh;
		for (uint8_t i = 0; i < cur.y; ++i)
			zero(i, c.w, 0);
		common_scr_erase(r, 0, cur.y - 1, mode);
		break;
	case 0:
		LOG("END");
		if (cur.y || cur.x) {
			r.y += (cur.y + 1) * fh;
			r.height = (c.height - cur.y - 1) * fh;
			for (uint8_t i = cur.y + 1; i < c.height; ++i)
				zero(i, c.w, 0);
			common_scr_erase(r, cur.y + 1, c.height - 1, mode);
			break;
		}
		/*  If we are positioned at the top left hand corner then
		 *  it is effectively a whole screen clear.
		 *  Drop through so that we do not need to duplicate
		 *  the scroll-up code.  */
	case 2:
	case 3: // for linux console compatibility
		LOG("ENTIRE");
		r.height = c.h - 1;
		scroll(0, r.height, r.height);
		common_scr_erase(r, 0, r.height, mode);
	}
}

