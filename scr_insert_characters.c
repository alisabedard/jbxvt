/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_insert_characters.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

//  Insert count spaces from the current position.
void scr_insert_characters(int16_t count)
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

