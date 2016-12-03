/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "edit.h"
#include <string.h>
#include "cursor.h"
#include "font.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "size.h"
#include "window.h"
#define DEBUG_EDIT
#ifndef DEBUG_EDIT
#undef LOG
#define LOG(...)
#endif//!DEBUG_EDIT
static void copy_area(xcb_connection_t * restrict xc,
	const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	const xcb_window_t v = jbxvt_get_vt_window(xc);
	xcb_copy_area(xc, v, v, jbxvt_get_text_gc(xc), x[0], y,
		x[1], y, width, jbxvt_get_font_size().height);
}
static void finalize(xcb_connection_t * restrict xc,
	const int16_t * restrict x, const struct JBDim p,
	const uint16_t width)
{
	copy_area(xc, x, p.y, width);
	jbxvt_get_current_screen()->wrap_next = 0;
	jbxvt_draw_cursor(xc);
}
static uint16_t get_width(const uint8_t count)
{
	const uint16_t w = jbxvt_get_char_size().w - count - jbxvt_get_x();
	return w * jbxvt_get_font_size().width;
}
static void set_x(int16_t * restrict x, const uint8_t count,
	const struct JBDim c)
{
	const int16_t p = jbxvt_chars_to_pixels(c).x;
	x[0] = p;
	x[1] = p + count * jbxvt_get_font_size().width;
}
static void begin(xcb_connection_t * restrict xc, int16_t * restrict x,
	const uint8_t count)
{
	jbxvt_draw_cursor(xc);
	set_x(x, count, jbxvt_get_current_screen()->cursor);
}
static struct JBDim get_cursor(void)
{
	return jbxvt_chars_to_pixels(jbxvt_get_current_screen()->cursor);
}
//  Insert count spaces from the current position.
void jbxvt_insert_characters(xcb_connection_t * xc, const uint8_t count)
{
	LOG("jbxvt_insert_characters(%d)", count);
	int16_t x[2];
	begin(xc, x, count);
	finalize(xc, x, get_cursor(), get_width(count));
}
//  Delete count characters from the current position.
void jbxvt_delete_characters(xcb_connection_t * xc, const uint8_t count)
{
	LOG("jbxvt_delete_characters(%d)", count);
	int16_t x[2];
	begin(xc, x, count);
	JB_SWAP(int16_t, x[0], x[1]);
	struct JBDim c = get_cursor();
	const uint16_t width = get_width(count);
	c.x += width;
	finalize(xc, x, c, width);
}
