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
	count = count < 0 ? 0 : count > jbxvt.scr.chars.width ?
		jbxvt.scr.chars.width : count;
	home_screen();
	cursor(CURSOR_DRAW);
	check_selection(jbxvt.scr.current->cursor.y,
		jbxvt.scr.current->cursor.y);
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->cursor.y];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->cursor.y];
	for (int16_t i = jbxvt.scr.chars.width - 1;
		i >= jbxvt.scr.current->cursor.x + count; --i) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
	const xcb_point_t p = {
		.x = MARGIN + jbxvt.scr.current->cursor.x
			* jbxvt.X.font_width,
		.y = MARGIN + jbxvt.scr.current->cursor.y
			* jbxvt.X.font_height
	};
	const uint16_t width = (jbxvt.scr.chars.width - count
		- jbxvt.scr.current->cursor.x) * jbxvt.X.font_width;
	if (width > 0) {
		  xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
			  jbxvt.X.gc.tx, p.x, p.y, p.x + count
			  * jbxvt.X.font_width, p.y, width,
			  jbxvt.X.font_height);
	}
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, p.x, p.y, count
		* jbxvt.X.font_width, jbxvt.X.font_height);
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

