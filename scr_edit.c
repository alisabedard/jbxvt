/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_edit.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

// Shortcuts:
#define CW jbxvt.scr.chars.width
#define FS jbxvt.X.f.size
#define SC jbxvt.scr.current
#define XC jbxvt.X.xcb
#define VT jbxvt.X.win.vt

static void copy_area(const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width <= 0)
		  return;
	xcb_copy_area(XC, VT, VT, jbxvt.X.gc.tx, x[0], y,
		x[1], y, width, FS.h);
}

static void finalize(const xcb_point_t p, const int8_t count)
{
	xcb_clear_area(XC, 0, VT, p.x, p.y, count * FS.w, FS.h);
	SC->wrap_next = 0;
	draw_cursor();
}

static void copy_lines(const int8_t count)
{
	const xcb_point_t c = SC->cursor;
	uint8_t * t = SC->text[c.y];
	uint32_t * r = SC->rend[c.y];
	for (int16_t i = jbxvt.scr.chars.width - 1;
		i >= c.x + count; --i) {
		t[i] = t[i - count];
		r[i] = r[i - count];
	}
}

//  Insert count spaces from the current position.
void scr_insert_characters(int8_t count)
{
	LOG("scr_insert_characters(%d)", count);
	count = MIN(MAX(count, 0), CW);
	change_offset(0);
	draw_cursor();
	const xcb_point_t c = SC->cursor;
	check_selection(c.y, c.y);
	copy_lines(count);
	const xcb_point_t p = { .x = MARGIN + c.x * FS.width,
		.y = MARGIN + c.y * FS.height };
	const uint16_t width = (CW - count - c.x) * FS.width;
	copy_area((int16_t[]){p.x, p.x + count * FS.width}, p.y, width);
	finalize(p, count);
}

static void copy_data_after_count(const uint8_t count, const xcb_point_t c)
{
	// copy the data after count
	const uint16_t offset = c.x + count;
	const uint16_t end = CW - c.x;
	uint8_t * t = SC->text[c.y];
	uint32_t * r = SC->rend[c.y];
#define MV(v) memmove(v + c.x, v + offset, (end - count) * sizeof(*v))
	MV(t); MV(r);
}

//  Delete count characters from the current position.
void scr_delete_characters(uint8_t count)
{
	LOG("scr_delete_characters(%d)", count);
	const xcb_point_t c = SC->cursor;
	const uint8_t end = CW - c.x;
	count = MIN(count, end); // keep within the screen
	if(!count)
		  return;
	change_offset(0);
	draw_cursor();
	copy_data_after_count(count, c);
	const int16_t y = MARGIN + c.y * FS.height;
	int16_t x[2] = {[1] = MARGIN + c.x * FS.width};
	x[0] = x[1] + count * FS.w;
	const uint16_t width = (end - count) * FS.w;
	copy_area(x, y, width);
	finalize((xcb_point_t){x[1] + width, y}, count);
}

