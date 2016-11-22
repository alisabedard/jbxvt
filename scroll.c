/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "scroll.h"
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
//#define SCROLL_DEBUG
#ifndef SCROLL_DEBUG
#undef LOG
#define LOG(...)
#endif//!SCROLL_DEBUG
static uint16_t scroll_top;
static struct JBXVTSavedLine saved_lines[JBXVT_MAX_SCROLL];
struct JBXVTSavedLine * jbxvt_get_saved_lines(void)
{
	return saved_lines;
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
	{ // sz scope
		const uint16_t sz = jbxvt_get_char_size().w;
		memset(text[count], 0, sz);
		memset(rend[count], 0, sz << 2);
	}
	{ // j scope, * s scope
		const uint8_t j = rc + (up ? - count - 1 : count);
		struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
		s->text[j] = text[count];
		s->rend[j] = rend[count];
	}
	clear(count, rc, text, rend, up);
}
static void adjust_saved_lines_top(const int_fast16_t n)
{
	scroll_top += n;
	scroll_top = JB_MIN(scroll_top, JBXVT_MAX_SCROLL);
}
static void copy_saved_lines(const int_fast16_t n)
{
	struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	for (int_fast16_t i = n - 1; i >= 0; --i) {
		struct JBXVTSavedLine * sl = saved_lines + n - i - 1;
		sl->wrap = s->wrap[i];
		sl->dwl = s->dwl[i];
		adjust_saved_lines_top(n);
		clear_selection_at(i);
		uint8_t * t = s->text[i];
		const size_t len = strlen((const char *)t);
		/* These are two distinct memory pools, so
		   use of memcpy here is valid.  */
		memcpy(sl->text, t, len);
		memcpy(sl->rend, s->rend[i], len << 2);
		sl->size = len;
	}
}
static void get_y(int16_t * restrict y, const uint8_t row1,
	const int8_t count, const bool up)
{
	const uint8_t fh = jbxvt_get_font_size().h;
	const int16_t a = row1 * fh;
	*(up ? y + 1 : y) = a;
	*(up ? y : y + 1) = a + count * fh;
}
static void copy_visible_area(xcb_connection_t * xc,
	const uint8_t row1, const uint8_t row2,
	const int8_t count, const bool up)
{
	int16_t y[2];
	get_y(y, row1, count, up);
	{ // vt scope
		const xcb_window_t vt = jbxvt_get_vt_window(xc);
		xcb_copy_area(xc, vt, vt, jbxvt_get_text_gc(xc), 0, y[0],
			0, y[1], jbxvt_get_pixel_size().width,
			(row2 - row1 - count) * jbxvt_get_font_size().h);
	}
	// the above blocks the event queue, flush it
	xcb_flush(xc);
}
static void add_scroll_history(void)
{
	int_fast16_t y = JBXVT_MAX_SCROLL - 2; // 2: 1 for i, 1 for j
	// i and j do not overlap since they are offset by 1
	for (struct JBXVTSavedLine * i = saved_lines + y,
		* j = i + 1; y >= 0; --y, --i, --j)
		memcpy(j, i, sizeof(struct JBXVTSavedLine));
	copy_saved_lines(1);
}
static int8_t copy_screen_area(const int8_t i,
	const int8_t j, const int8_t mod, const int8_t count,
	uint8_t ** save, uint32_t ** rend)
{
	if(i >= count)
		  return j;
	{ // * s scope
		struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
		save[i] = s->text[j];
		rend[i] = s->rend[j];
	}
	clear_selection_at(j);
	return copy_screen_area(i + 1, j + mod, mod, count, save, rend);
}
static void clear_line(xcb_connection_t * xc,
	const int16_t y, const int8_t count)
{
	const uint8_t fh = jbxvt_get_font_size().height;
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), 0, y * fh,
		jbxvt_get_pixel_size().width, count * fh);
}
void jbxvt_scroll_primary_screen(int16_t n)
{
	LOG("jbxvt_scroll_primary_screen(%d)", n);
	copy_saved_lines(n);
	struct JBXVTScreen * s = jbxvt_get_screen_at(0);
	const uint16_t h = jbxvt_get_char_size().height;
	for (int_fast16_t j = n; j < h; ++j)
		move_line(j, -n, s);
}
static void sc_common(xcb_connection_t * xc,
	const uint8_t r1, const uint8_t r2,
	const int16_t count, const bool up,
	uint8_t ** save, uint32_t ** rend)
{
	clear(count, up ? r2 : r1, save, rend, up);
	copy_visible_area(xc, r1, r2, count, up);
	clear_line(xc, up ? (r2 - count) : r1, count);
	jbxvt_draw_scrollbar(xc);
}
static void sc_dn(xcb_connection_t * xc,
	const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	{ // * s scope
		struct JBXVTScreen * s = jbxvt_get_current_screen();
		for(int8_t j = copy_screen_area(0, row2, -1,
			count, save, rend); j >= row1; --j)
			  move_line(j, count, s);
	}
	sc_common(xc, row1, row2, count, false, save, rend);
}
static void sc_up(xcb_connection_t * xc,
	const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	{ // * s scope
		struct JBXVTScreen * s = jbxvt_get_current_screen();
		if (s == jbxvt_get_screen_at(0) && row1 == 0)
			add_scroll_history();
		for(int8_t j = copy_screen_area(0, row1,
			1, count, save, rend); j < row2; ++j)
			move_line(j, -count, s);
	}
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
	{ // abs_count scope
		const uint16_t abs_count = abs(count);
		uint8_t *save[abs_count];
		uint32_t *rend[abs_count];
		(count > 0 ? sc_up : sc_dn)(xc, row1, row2 + 1,
			abs_count, save, rend);
	}
	jbxvt_set_scroll(xc, 0);
}
