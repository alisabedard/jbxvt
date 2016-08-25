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

//#define DEBUG_ERASE
#ifndef DEBUG_ERASE
#undef LOG
#define LOG(...)
#endif

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
	const uint16_t w = jbxvt.X.f.size.width;
	h->x = MARGIN + col * w;
	h->width = sz * w;
}

//  erase part or the whole of a line
void scr_erase_line(const int8_t mode)
{
	LOG("scr_erase_line(%d)", mode);
	change_offset(0);
	VTScreen * scr = jbxvt.scr.current;
	struct JBDim c = scr->cursor;
	const uint8_t fh = jbxvt.X.f.size.height;
	xcb_rectangle_t g = { .y = MARGIN + c.y * fh };
	const uint8_t cw = jbxvt.scr.chars.width;
	switch (mode) {
#define EL(msg, a, b) LOG(msg); get_horz_geo(&g, a, b); zero(c.y, a, b);
	case 1:
		EL("START", c.x, 0);
		break;
	case 0:
		EL("END", cw - c.x, c.x);
		break;
	case 2:
		EL("ENTIRE", cw, 0);
		break;
	}
	draw_cursor(); //clear
	check_selection(c.y, c.y);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
		g.x, g.y, g.width, fh);
	scr->wrap_next = false;
	draw_cursor();
	xcb_flush(jbxvt.X.xcb);
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
#define CSZ jbxvt.scr.chars
#define PSZ jbxvt.scr.pixels
#define FH jbxvt.X.f.size.height
	xcb_rectangle_t r = {.x = MARGIN, .y = MARGIN,
		.width = PSZ.width};
	const struct JBDim cur = s->cursor;
	switch (mode) {
	case 1:
		LOG("START");
		r.height = cur.y * FH;
		for (uint8_t i = 0; i < cur.y; ++i)
			zero(i, CSZ.w, 0);
		common_scr_erase(r, 0, cur.y - 1, mode);
		break;
	case 0:
		LOG("END");
		if (cur.y || cur.x) {
			const int16_t c1 = cur.y + 1;
			r.y += c1 * FH;
			r.height = (CSZ.height - cur.y - 1) * FH;
			for (uint8_t i = c1; i < CSZ.height; ++i)
				zero(i, CSZ.w, 0);
			common_scr_erase(r, c1, CSZ.height - 1, mode);
			break;
		}
		/*  If we are positioned at the top left hand corner then
		 *  it is effectively a whole screen clear.
		 *  Drop through so that we do not need to duplicate
		 *  the scroll-up code.  */
	case 2:
	case 3: // for linux console compatibility
		LOG("ENTIRE");
		r.height = CSZ.h - 1;
		scroll(0, r.height, r.height);
		common_scr_erase(r, 0, r.height, mode);
	}
}

