/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "scroll.h"
#include "font.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "paint.h"
#include "sbar.h"
#include "selection.h"
#include "size.h"
#include "window.h"
#include <string.h>
//#define SCROLL_DEBUG
#ifndef SCROLL_DEBUG
#undef LOG
#define LOG(...)
#endif//!SCROLL_DEBUG
static uint16_t scroll_top, scroll_max = JBXVT_MAX_SCROLL;
static struct JBXVTSavedLine saved_lines[JBXVT_MAX_SCROLL];
struct JBXVTSavedLine * jbxvt_get_saved_lines(void)
{
	return saved_lines;
}
void jbxvt_set_scroll_max(const uint16_t val)
{
	scroll_max = val;
}
void jbxvt_zero_scroll_top(void)
{
	scroll_top = 0;
}
uint16_t jbxvt_get_scroll_top(void)
{
	return scroll_top;
}
static void clear_selection_at(const int16_t j)
{
	struct JBDim * e = jbxvt_get_selection_end_points();
	if (e[0].index == j || e[1].index == j)
		jbxvt_clear_selection();
}
static void move_line(const int16_t j,
	const int8_t count, struct JBXVTScreen * restrict s)
{
	const int16_t k = j + count;
	s->text[k] = s->text[j];
	s->rend[k] = s->rend[j];
	clear_selection_at(j);
}
static void clear(int8_t count, const uint8_t rc,
	uint8_t ** text, uint32_t ** rend, const bool up)
{
	if(--count < 0)
		  return;
	memset(text[count], 0, jbxvt_get_char_size().w);
	memset(rend[count], 0, jbxvt_get_char_size().w << 2);
	const uint8_t j = rc + (up ? - count - 1 : count);
	jbxvt.scr.current->text[j] = text[count];
	jbxvt.scr.current->rend[j] = rend[count];
	clear(count, rc, text, rend, up);
}
static void adjust_saved_lines_top(const int_fast16_t n)
{
	scroll_top += n;
	scroll_top = JB_MIN(scroll_top,
		scroll_max);
}
static void copy_saved_lines(const int_fast16_t n)
{
	for (int_fast16_t i = n - 1; i >= 0; --i) {
		uint8_t * t = jbxvt.scr.current->text[i];
		struct JBXVTSavedLine * sl = saved_lines + n - i - 1;
		sl->wrap = jbxvt.scr.current->wrap[i];
		sl->dwl = jbxvt.scr.current->dwl[i];
		adjust_saved_lines_top(n);
		clear_selection_at(i);
		const size_t len = strlen((const char *)t);
		memcpy(sl->text, t, len);
		memcpy(sl->rend, jbxvt.scr.current->rend[i], len << 2);
		sl->size = len;
	}
}
static void get_y(int16_t * restrict y, const uint8_t row1,
	const int8_t count, const bool up)
{
	const int16_t a = row1 * jbxvt_get_font_size().h;
	*(up ? y + 1 : y) = a;
	*(up ? y : y + 1) = a + count * jbxvt_get_font_size().h;
}
static void copy_visible_area(xcb_connection_t * xc,
	const uint8_t row1, const uint8_t row2,
	const int8_t count, const bool up)
{
	int16_t y[2];
	get_y(y, row1, count, up);
	const uint16_t height = (row2 - row1 - count)
		* jbxvt_get_font_size().h;
	xcb_copy_area(xc, jbxvt_get_vt_window(xc),
		jbxvt_get_vt_window(xc), jbxvt_get_text_gc(xc), 0, y[0],
		0, y[1], jbxvt_get_pixel_size().width, height);
	// the above blocks the event queue, flush it
	xcb_flush(xc);
}
static void add_scroll_history(xcb_connection_t * xc,
	const int8_t count)
{
	if (count < 1) // nothing to do
		return;
	// Handle lines that scroll off the top of the screen.
	memcpy(saved_lines + count, saved_lines,
		count - 1); // -1 to avoid going over array bounds
	int_fast16_t y = scroll_max - count - 1;
	struct JBXVTSavedLine * i = &saved_lines[y],
		* j = &saved_lines[y + count];
	for (; y >= 0; --y, --i, --j)
		memcpy(j, i, sizeof(struct JBXVTSavedLine));
	copy_saved_lines(count);
	jbxvt_draw_scrollbar(xc);
}
static int8_t copy_screen_area(const int8_t i,
	const int8_t j, const int8_t mod, const int8_t count,
	uint8_t ** save, uint32_t ** rend)
{
	if(i >= count)
		  return j;
	save[i] = jbxvt.scr.current->text[j];
	rend[i] = jbxvt.scr.current->rend[j];
	clear_selection_at(j);
	return copy_screen_area(i + 1, j + mod, mod,
		count, save, rend);
}
static void clear_line(xcb_connection_t * xc,
	const int16_t y, const int8_t count)
{
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), 0,
		y * jbxvt_get_font_size().height,
		jbxvt_get_pixel_size().width,
		count * jbxvt_get_font_size().height);
}
void jbxvt_scroll_primary_screen(int16_t n)
{
	LOG("jbxvt_scroll_primary_screen(%d)", n);
	copy_saved_lines(n);
	for (int_fast16_t j = n;
		j < jbxvt_get_char_size().height; ++j)
		  move_line(j, -n, &jbxvt.scr.s[0]);
}
static void sc_common(xcb_connection_t * xc,
	const uint8_t r1, const uint8_t r2,
	const int16_t count, const bool up,
	uint8_t ** save, uint32_t ** rend)
{
	clear(count, up ? r2 : r1, save, rend, up);
	copy_visible_area(xc, r1, r2, count, up);
	clear_line(xc, up ? (r2 - count) : r1, count);
}
static void sc_dn(xcb_connection_t * xc,
	const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	for(int8_t j = copy_screen_area(0, row2, -1,
		count, save, rend); j >= row1; --j)
		  move_line(j, count, jbxvt.scr.current);
	sc_common(xc, row1, row2, count, false, save, rend);
}
static void sc_up(xcb_connection_t * xc,
	const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	if (jbxvt.scr.current == &jbxvt.scr.s[0] && row1 == 0)
		add_scroll_history(xc, count);
	for(int8_t j = copy_screen_area(0, row1,
		1, count, save, rend); j < row2; ++j)
		move_line(j, -count, jbxvt.scr.current);
	sc_common(xc, row1, row2, count, true, save, rend);
}
/*  Scroll count lines from row1 to row2 inclusive.
    row1 should be <= row2.  Scrolling is up for
    a positive count and down for a negative count.
    count is limited to a maximum of SCROLL lines.  */
void scroll(xcb_connection_t * xc, const uint8_t row1,
	const uint8_t row2, const int16_t count)
{
	LOG("scroll(%d, %d, %d)", row1, row2, count);
	if (!count)
		return;
	const uint16_t abs_count = abs(count);
	uint8_t *save[abs_count];
	uint32_t *rend[abs_count];
	(count > 0 ? sc_up : sc_dn)(xc, row1, row2 + 1,
		abs_count, save, rend);
	jbxvt_set_scroll(xc, 0);
}
