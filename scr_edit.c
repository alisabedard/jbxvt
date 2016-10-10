/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_edit.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"

#include <string.h>

static void copy_area(const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width > 0)
		xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
			jbxvt.X.gc.tx, x[0], y, x[1], y, width,
			jbxvt.X.font.size.height);
}

static void finalize(const int16_t * restrict x, const struct JBDim p,
	const uint16_t width, const int8_t count)
{
	copy_area(x, p.y, width);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, p.x, p.y,
		count * jbxvt.X.font.size.w, jbxvt.X.font.size.h);
	jbxvt.scr.current->wrap_next = 0;
	draw_cursor();
}

static void copy_lines(const int16_t x, const int8_t count)
{
	const int16_t y = jbxvt.scr.current->cursor.y;
	uint8_t * s = jbxvt.scr.current->text[y];
	uint32_t * r = jbxvt.scr.current->rend[y];
	for (int16_t i = jbxvt.scr.chars.w - 1; i >= x + count; --i) {
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
	count = MAX(count, 0);
	count = MIN(count, insert ? jbxvt.scr.chars.w
		: jbxvt.scr.chars.w - jbxvt.scr.current->cursor.x);
	return count;
}

static void begin(int16_t * x, int8_t * restrict count, const bool insert)
{
	*count = get_count(*count, insert);
	jbxvt_set_scroll(0);
	draw_cursor();
	const struct JBDim c = jbxvt.scr.current->cursor;
	struct JBDim p = jbxvt_get_pixel_size(c);
	x[0] = p.x;
	x[1] = p.x + *count * jbxvt.X.font.size.width;
	if (!insert)
		JB_SWAP(int16_t, x[0], x[1]);
	jbxvt_check_selection(c.y, c.y);
}

//  Insert count spaces from the current position.
void jbxvt_insert_characters(int8_t count)
{
	LOG("jbxvt_insert_characters(%d)", count);
	int16_t x[2];
	begin(x, &count, true);
	const struct JBDim c = jbxvt.scr.current->cursor;
	copy_lines(c.x, count);
	finalize(x, jbxvt_get_pixel_size(c), get_width(count), count);
}

static void move(void * data, const int16_t x, const int16_t offset,
	const int16_t diff, const size_t n)
{
	memmove(data + x, data + offset, n * diff);
}

static void copy_data_after_count(const uint8_t count, const struct JBDim c)
{
	// copy the data after count
	const int16_t end = jbxvt.scr.chars.width - c.x;
	const int16_t diff = end - count;
	const int16_t offset = c.x + count;
	move (jbxvt.scr.current->text[c.y], c.x, offset,
		diff, sizeof(uint8_t));
	move (jbxvt.scr.current->text[c.y], c.x, offset,
		diff, sizeof(uint32_t));
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
void jbxvt_delete_characters(int8_t count)
{
	LOG("jbxvt_delete_characters(%d)", count);
	int16_t x[2];
	begin(x, &count, false);
	struct JBDim c = jbxvt.scr.current->cursor;
	copy_data_after_count(count, c);
	delete_source_data(count, c.y);
	c = jbxvt_get_pixel_size(c);
	const uint16_t width = get_width(count);
	c.x += width;
	finalize(x, c, width, count);
}

