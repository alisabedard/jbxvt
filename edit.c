/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "edit.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include <string.h>
static void copy_area(xcb_connection_t * xc,
	const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width > 0)
		xcb_copy_area(xc, jbxvt.X.win.vt, jbxvt.X.win.vt,
			jbxvt_get_text_gc(xc), x[0], y, x[1], y, width,
			jbxvt.X.font.size.height);
}
static void finalize(xcb_connection_t * xc,
	const int16_t * restrict x, const struct JBDim p,
	const uint16_t width, const int8_t count)
{
	copy_area(xc, x, p.y, width);
	xcb_clear_area(xc, 0, jbxvt.X.win.vt, p.x, p.y,
		count * jbxvt.X.font.size.w, jbxvt.X.font.size.h);
	jbxvt.scr.current->wrap_next = 0;
	jbxvt_draw_cursor(xc);
}
static void copy_lines(const int16_t x, const int8_t count)
{
	const int16_t y = jbxvt.scr.current->cursor.y;
	uint8_t * s = jbxvt.scr.current->text[y];
	uint32_t * r = jbxvt.scr.current->rend[y];
	for (int_fast16_t i = x + count; i < jbxvt.scr.chars.w; ++i) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
}
static uint16_t get_width(const uint8_t count)
{
	return (jbxvt.scr.chars.w - count - jbxvt.scr.current->cursor.x)
		* jbxvt.X.font.size.w;
}
static uint8_t get_count(int8_t count, const bool insert)
{
	count = JB_MAX(count, 0);
	count = JB_MIN(count, insert ? jbxvt.scr.chars.w
		: jbxvt.scr.chars.w - jbxvt.scr.current->cursor.x);
	return count;
}
static void begin(xcb_connection_t * xc, int16_t * x,
	int8_t * restrict count, const bool insert)
{
	*count = get_count(*count, insert);
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc);
	const struct JBDim c = jbxvt.scr.current->cursor;
	struct JBDim p = jbxvt_get_pixel_size(c);
	x[0] = p.x;
	x[1] = p.x + *count * jbxvt.X.font.size.width;
	if (!insert)
		JB_SWAP(int16_t, x[0], x[1]);
	jbxvt_check_selection(xc, c.y, c.y);
}
//  Insert count spaces from the current position.
void jbxvt_insert_characters(xcb_connection_t * xc, int8_t count)
{
	LOG("jbxvt_insert_characters(%d)", count);
	int16_t x[2];
	begin(xc, x, &count, true);
	const struct JBDim c = jbxvt.scr.current->cursor;
	copy_lines(c.x, count);
	finalize(xc, x, jbxvt_get_pixel_size(c), get_width(count), count);
}
static void copy_data_after_count(const uint8_t count, const struct JBDim c)
{
	// copy the data after count
	const uint16_t diff = jbxvt.scr.chars.width - count;
	{
		uint8_t * i = jbxvt.scr.current->text[c.y] + c.x;
		memmove(i, i + count, diff);
	}
	{
		uint32_t * i = jbxvt.scr.current->rend[c.y] + c.x;
		memmove(i, i + count, diff << 2);
	}
}
static void delete_source_data(const uint8_t count, const int16_t y)
{
	// delete the source data copied
	memset(jbxvt.scr.current->text[y] + jbxvt.scr.chars.w - count,
		0, count);
	memset(jbxvt.scr.current->rend[y] + jbxvt.scr.chars.w - count,
		0, count << 2);
}
//  Delete count characters from the current position.
void jbxvt_delete_characters(xcb_connection_t * xc, int8_t count)
{
	LOG("jbxvt_delete_characters(%d)", count);
	int16_t x[2];
	begin(xc, x, &count, false);
	struct JBDim c = jbxvt.scr.current->cursor;
	copy_data_after_count(count, c);
	delete_source_data(count, c.y);
	c = jbxvt_get_pixel_size(c);
	const uint16_t width = get_width(count);
	c.x += width;
	finalize(xc, x, c, width, count);
}
