/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_edit.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"

#include <string.h>

#define CUR SCR->cursor
#define END (CSZ.w - CUR.x)
#define VT jbxvt.X.win.vt

static void copy_area(const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width > 0)
		xcb_copy_area(XC, VT, VT, jbxvt.X.gc.tx, x[0], y,
			x[1], y, width, FSZ.height);
}

static void finalize(const int16_t * restrict x, const struct JBDim p,
	const uint16_t width, const int8_t count)
{
	copy_area(x, p.y, width);
	xcb_clear_area(XC, 0, VT, p.x, p.y, count * FSZ.w, FSZ.h);
	SCR->wrap_next = 0;
	draw_cursor();
}

static void copy_lines(const int16_t x, const int8_t count)
{
	const int16_t y = SCR->cursor.y;
	uint8_t * s = SCR->text[y];
	uint32_t * r = SCR->rend[y];
	for (int16_t i = CSZ.w - 1; i >= x + count; --i) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
}

static uint16_t get_width(const uint8_t count, const bool insert)
{
	return (insert ? CSZ.w - count - CUR.x : END - count) * FSZ.w;
}

static uint8_t get_count(int8_t count, const bool insert)
{
	count = MAX(count, 0);
	count = MIN(count, insert ? CSZ.w : CSZ.w - CUR.x);
	return count;
}

static void begin(int16_t * x, int8_t * restrict count, const bool insert)
{
	*count = get_count(*count, insert);
	change_offset(0);
	draw_cursor();
	const struct JBDim c = CUR;
	struct JBDim p = get_p(c);
	x[0] = p.x;
	x[1] = p.x + *count * FSZ.width;
	if (!insert)
		JB_SWAP(int16_t, x[0], x[1]);
	check_selection(c.y, c.y);
}

//  Insert count spaces from the current position.
void scr_insert_characters(int8_t count)
{
	LOG("scr_insert_characters(%d)", count);
	int16_t x[2];
	begin(x, &count, true);
	const struct JBDim c = SCR->cursor;
	copy_lines(c.x, count);
	finalize(x, get_p(c), get_width(count, true), count);
}

static void copy_data_after_count(const uint8_t count, const struct JBDim c)
{
	// copy the data after count
	const uint16_t offset = c.x + count;
	const uint16_t end = CSZ.width - c.x;
	uint8_t * t = SCR->text[c.y];
	uint32_t * r = SCR->rend[c.y];
	memmove(t + c.x, t + offset, end - count);
	memmove(r + c.x, r + offset,
		(end - count) * sizeof(uint32_t));
}

static void delete_source_data(const uint8_t count, const int16_t y)
{
	uint8_t * t = SCR->text[y];
	uint32_t * r = SCR->rend[y];
	// delete the source data copied
	memset(t + CSZ.w - count, 0, count);
	memset(r + CSZ.w - count, 0, count << 2);
}

//  Delete count characters from the current position.
void scr_delete_characters(int8_t count)
{
	LOG("scr_delete_characters(%d)", count);
	int16_t x[2];
	begin(x, &count, false);
	struct JBDim c = CUR;
	copy_data_after_count(count, c);
	delete_source_data(count, c.y);
	c = get_p(c);
	const uint16_t width = get_width(count, false);
	c.x += width;
	finalize(x, c, width, count);
}

