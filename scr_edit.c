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

static void copy_area(const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width <= 0)
		  return;
	const xcb_window_t v = jbxvt.X.win.vt;
	xcb_copy_area(jbxvt.X.xcb, v, v, jbxvt.X.gc.tx, x[0], y,
		x[1], y, width, jbxvt.X.f.size.height);
}

static void finalize(const xcb_point_t p, const int8_t count)
{
	const Size f = jbxvt.X.f.size;
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
		p.x, p.y, count * f.w, f.h);
	jbxvt.scr.current->wrap_next = 0;
	draw_cursor();
}

static void copy_lines(const int16_t x, const uint8_t cw,
	const int8_t count)
{
	VTScreen * restrict scr = jbxvt.scr.current;
	const xcb_point_t c = scr->cursor;
	uint8_t * s = scr->text[c.y];
	uint32_t * r = scr->rend[c.y];

	for (int16_t i = cw - 1; i >= x + count; --i) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
}

//  Insert count spaces from the current position.
void scr_insert_characters(int8_t count)
{
	LOG("scr_insert_characters(%d)", count);
	count = MAX(count, 0);
	const uint8_t cw = jbxvt.scr.chars.width;
	count = MIN(count, cw);
	change_offset(0);
	draw_cursor();
	VTScreen * restrict scr = jbxvt.scr.current;
	const xcb_point_t c = scr->cursor;
	check_selection(c.y, c.y);
	copy_lines(c.x, cw, count);
	const Size f = jbxvt.X.f.size;
	const xcb_point_t p = { .x = MARGIN + c.x * f.width,
		.y = MARGIN + c.y * f.height };
	const uint16_t width = (cw - count - c.x) * f.width;
	copy_area((int16_t[]){p.x, p.x + count * f.width}, p.y, width);
	finalize(p, count);
}

static void copy_data_after_count(const uint8_t count, const xcb_point_t c)
{
	// copy the data after count
	const uint16_t offset = c.x + count;
	const uint16_t end = jbxvt.scr.chars.width - c.x;
	VTScreen * scr = jbxvt.scr.current;
	uint8_t * t = scr->text[c.y];
	uint32_t * r = scr->rend[c.y];
	memmove(t + c.x, t + offset, end - count);
	memmove(r + c.x, r + offset,
		(end - count) * sizeof(uint32_t));
}

static void delete_source_data(const uint8_t count, const int16_t y)
{
	const uint16_t scw = jbxvt.scr.chars.width;
	VTScreen * scr = jbxvt.scr.current;
	uint8_t * t = scr->text[y];
	uint32_t * r = scr->rend[y];
	// delete the source data copied
	memset(t + scw - count, 0, count);
	memset(r + scw - count, 0, count << 2);
}

//  Delete count characters from the current position.
void scr_delete_characters(uint8_t count)
{
	LOG("scr_delete_characters(%d)", count);
	const uint8_t scw = jbxvt.scr.chars.width;
	VTScreen * scr = jbxvt.scr.current;
	const xcb_point_t c = scr->cursor;
	const uint8_t end = scw - c.x;
	count = MIN(count, end); // keep within the screen
	if(!count)
		  return;
	change_offset(0);
	draw_cursor();
	copy_data_after_count(count, c);
	delete_source_data(count, c.y);
	const Size f = jbxvt.X.f.size;
	const int16_t y = MARGIN + c.y * f.height;
	int16_t x[2] = {[1] = MARGIN + c.x * f.width};
	x[0] = x[1] + count * f.w;
	const uint16_t width = (end - count) * f.w;
	copy_area(x, y, width);
	finalize((xcb_point_t){x[1] + width, y}, count);
}

