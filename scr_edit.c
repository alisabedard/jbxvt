/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_edit.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

//  Insert count spaces from the current position.
void scr_insert_characters(int8_t count)
{
	LOG("scr_insert_characters(%d)", count);
	count = MAX(count, 0);
	const uint8_t cw = jbxvt.scr.chars.width;
	count = MIN(count, cw);
	home_screen();
	cursor(CURSOR_DRAW);
	VTScreen * restrict scr = jbxvt.scr.current;
	const xcb_point_t c = scr->cursor;
	check_selection(c.y, c.y);
	uint8_t * s = scr->text[c.y];
	uint32_t * r = scr->rend[c.y];
	for (int16_t i = cw - 1; i >= c.x + count; --i) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
	const Size f = { .w = jbxvt.X.font_width, .h = jbxvt.X.font_height};
	const xcb_point_t p = { .x = MARGIN + c.x * f.width,
		.y = MARGIN + c.y * f.height };
	const uint16_t width = (cw - count - c.x) * f.width;
	if (width > 0) {
		  xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
			  jbxvt.X.gc.tx, p.x, p.y, p.x + count
			  * f.width, p.y, width,
			  f.height);
	}
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, p.x, p.y,
		count * f.width, f.height);
	scr->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

static void copy_area(const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width > 0) {
		xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
			jbxvt.X.gc.tx, x[0], y, x[1], y, width,
			jbxvt.X.font_height);
	}
}

//  Delete count characters from the current position.
void scr_delete_characters(uint8_t count)
{
	LOG("scr_delete_characters(%d)", count);
	const uint8_t scw = jbxvt.scr.chars.width;
	VTScreen * restrict scr = jbxvt.scr.current;
	const xcb_point_t c = scr->cursor;
	count = MIN(count, scw - c.x); // keep within the screen
	if(!count) return;
	home_screen();
	cursor(CURSOR_DRAW);
	uint8_t * s = scr->text[c.y];
	uint32_t * r = scr->rend[c.y];

	// copy the data after count
	memmove(s + c.x, s + c.x + count, scw - c.x - count);
	memmove(r + c.x, r + c.x + count,
		(scw - c.x - count) * sizeof(uint32_t));
	// delete the source data copied
	memset(s + scw - count, 0, count);
	memset(r + scw - count, 0, count * sizeof(uint32_t));

	const Size f = { .w = jbxvt.X.font_width,
		.h = jbxvt.X.font_height };
	const int16_t y = MARGIN + c.y * f.height;
	int16_t x[2] = {[1] = MARGIN + c.x * f.width};
	x[0] = x[1] + count * f.w;
	const uint16_t width = (scw - count - c.x) * f.w;
	copy_area(x, y, width);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, x[1] + width, y,
		count * f.w, f.height);
	scr->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

