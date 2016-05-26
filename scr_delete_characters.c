/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_delete_characters.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repair_damage.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

static void copy_area(const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width > 0) {
		XCopyArea(jbxvt.X.dpy, jbxvt.X.win.vt, jbxvt.X.win.vt,
			jbxvt.X.gc.tx, x[0], y, width,
			jbxvt.X.font_height, x[1], y);
		repair_damage();
	}
}

//  Delete count characters from the current position.
void scr_delete_characters(int count)
{
	LOG("scr_delete_characters(%d)", count);
	const uint8_t scw = jbxvt.scr.chars.width;
	const Dim c = { // current cursor position
		.col = jbxvt.scr.current->col,
		.row = jbxvt.scr.current->row
	};
	count = constrain(count, scw - c.row + 1);
	if(!count) return;
	home_screen();
	cursor(CURSOR_DRAW);
	check_selection(c.r, c.r);
	uint8_t * s = jbxvt.scr.current->text[c.row];
	uint32_t * r = jbxvt.scr.current->rend[c.row];
	for (uint8_t i = c.col + count; i < scw; i++) {
		s[i - count] = s[i];
		r[i - count] = r[i];
	}
	memset(s + scw - count, 0, count);
	memset(r + scw - count, 0, count);
	const Dim f = { .w = jbxvt.X.font_width,
		.h = jbxvt.X.font_height };
	const int16_t y = MARGIN + c.row * f.height;
	int16_t x[2] = {[1] = MARGIN + c.col * f.width};
	x[0] = x[1] + count * f.w;
	const uint16_t width = (scw - count - c.col) * f.w;
	copy_area(x, y, width);
	XClearArea(jbxvt.X.dpy, jbxvt.X.win.vt, x[1] + width, y,
		count * jbxvt.X.font_width, f.height, false);
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

