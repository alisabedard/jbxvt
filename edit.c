/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "edit.h"
#include "cursor.h"
#include "font.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "size.h"
#include "window.h"
#include <string.h>
static void copy_area(xcb_connection_t * restrict xc,
	const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	const xcb_window_t v = jbxvt_get_vt_window(xc);
	xcb_copy_area(xc, v, v, jbxvt_get_text_gc(xc), x[0], y,
		x[1], y, width, jbxvt_get_font_size().height);
}
static void clear_area(xcb_connection_t * restrict xc, struct JBDim p,
	const int8_t count)

{
	LOG("clear_area(xc, p = {.x = %d, .y = %d}, count = %d)",
		p.x, p.y, count);
	const struct JBDim f = jbxvt_get_font_size();
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc),
		p.x, p.y, count * f.w, f.h);
}
static void finalize(xcb_connection_t * restrict xc,
	const int16_t * restrict x, const struct JBDim p,
	const uint16_t width, const int8_t count)
{
	copy_area(xc, x, p.y, width);
	clear_area(xc, p, count);
	jbxvt_get_screen()->wrap_next = 0;
	jbxvt_draw_cursor(xc);
}
static void copy_lines(const int16_t x, const int8_t count)
{
	struct JBXVTScreen * restrict screen = jbxvt_get_screen();
	const int16_t y = screen->cursor.y;
	uint8_t * t = screen->text[y];
	uint32_t * r = screen->rend[y];
	for (int_fast16_t w = jbxvt_get_char_size().w,
		i = x + count; i < w; ++i) {
		const int_fast16_t j = i - count;
		t[i] = t[j];
		r[i] = r[j];
	}
}
static uint16_t get_width(const uint8_t count)
{
	const uint16_t w = jbxvt_get_char_size().w;
	return (w - count - jbxvt_get_screen()->cursor.x) * w;
}
static uint8_t get_count(int8_t count, const bool insert)
{
	count = JB_MAX(count, 0);
	count = JB_MIN(count, insert ? jbxvt_get_char_size().w
		: jbxvt_get_char_size().w
		- jbxvt_get_screen()->cursor.x);
	return count;
}
static void set_x(int16_t * restrict x, const int8_t count,
	const struct JBDim c)
{
	const int16_t p = jbxvt_chars_to_pixels(c).x;
	x[0] = p;
	x[1] = p + count * jbxvt_get_font_size().width;
}
static void begin(xcb_connection_t * xc, int16_t * restrict x,
	int8_t * restrict count, const bool insert)
{
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc);
	const struct JBDim c = jbxvt_get_screen()->cursor;
	set_x(x, *count = get_count(*count, insert), c);
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
	const struct JBDim c = jbxvt_get_screen()->cursor;
	copy_lines(c.x, count);
	finalize(xc, x, jbxvt_chars_to_pixels(c),
		get_width(count), count);
}
static void copy_data_after_count(const uint8_t count,
	const struct JBDim c)
{
	// copy the data after count
	const uint16_t diff = jbxvt_get_char_size().width - count;
	struct JBXVTScreen * restrict s = jbxvt_get_screen();
	{ // * i scope
		uint8_t * i = s->text[c.y] + c.x;
		memmove(i, i + count, diff);
	}
	{ // * i scope
		uint32_t * i = s->rend[c.y] + c.x;
		memmove(i, i + count, diff << 2);
	}
}
static void delete_source_data(const uint8_t count, const int16_t y)
{
	// delete the source data copied
	const uint16_t c = jbxvt_get_char_size().w - count;
	struct JBXVTScreen * restrict s = jbxvt_get_screen();
	memset(s->text[y] + c, 0, count);
	memset(s->rend[y] + c, 0, count << 2);
}
//  Delete count characters from the current position.
void jbxvt_delete_characters(xcb_connection_t * xc, int8_t count)
{
	LOG("jbxvt_delete_characters(%d)", count);
	int16_t x[2];
	begin(xc, x, &count, false);
	struct JBDim c = jbxvt_get_screen()->cursor;
	copy_data_after_count(count, c);
	delete_source_data(count, c.y);
	c = jbxvt_chars_to_pixels(c);
	const uint16_t width = get_width(count);
	c.x += width;
	finalize(xc, x, c, width, count);
}
