/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_delete_characters.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

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
void scr_delete_characters(int count)
{
	LOG("scr_delete_characters(%d)", count);
	const uint8_t scw = jbxvt.scr.chars.width;
	const xcb_point_t c = jbxvt.scr.current->cursor;
	if (count > scw - c.y)
		  count = scw - c.y;
	if(!count) return;
	home_screen();
	cursor(CURSOR_DRAW);
	uint8_t * s = jbxvt.scr.current->text[c.y];
	uint32_t * r = jbxvt.scr.current->rend[c.y];
	memmove(s + c.x, s + c.x + count, count);
	memmove(r + c.x, r + c.x + count, count * sizeof(uint32_t));
	memset(s + scw - count, 0, count);
	memset(r + scw - count, 0, count);
	const Size f = { .w = jbxvt.X.font_width,
		.h = jbxvt.X.font_height };
	const int16_t y = MARGIN + c.y * f.height;
	int16_t x[2] = {[1] = MARGIN + c.x * f.width};
	x[0] = x[1] + count * f.w;
	const uint16_t width = (scw - count - c.x) * f.w;
	copy_area(x, y, width);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, x[1] + width, y,
		count * jbxvt.X.font_width, f.height);
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

