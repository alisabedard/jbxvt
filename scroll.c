/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#define LOG_LEVEL 3
#include "scroll.h"
#include <string.h>
#include "JBXVTLine.h"
#include "JBXVTScreen.h"
#include "font.h"
#include "libjb/JBDim.h"
#if LOG_LEVEL > 0
#include "libjb/log.h"
#endif//LOG_LEVEL
#include "libjb/macros.h"
#include "libjb/util.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "size.h"
#include "window.h"
static struct JBXVTLine * saved_lines;
static uint16_t scroll_size;
struct JBXVTLine * jbxvt_get_saved_lines(void)
{
	return saved_lines;
}
void jbxvt_clear_scroll_history(void)
{
	free(saved_lines);
	saved_lines = NULL;
	scroll_size = 0;
}
uint16_t jbxvt_get_scroll_size(void)
{
	return scroll_size;
}
static void clear_selection_at(const int16_t j)
{
	struct JBDim * e = jbxvt_get_selection_end_points();
	if (e[0].index == j || e[1].index == j)
		jbxvt_clear_selection();
}
static void move_line(const int16_t j, const int8_t count,
	struct JBXVTScreen * restrict s)
{
	const uint16_t k = j + count;
	struct JBXVTLine * dest = s->line + k, * src = s->line + j;
	memcpy(dest, src, sizeof(struct JBXVTLine));
	clear_selection_at(j);
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
		xcb_copy_area(xc, vt, vt, jbxvt_get_text_gc(xc), 0,
			y[0], 0, y[1], jbxvt_get_pixel_size().width,
			(row2 - row1 - count) *
			jbxvt_get_font_size().h);
	}
	// the above blocks the event queue, flush it
	xcb_flush(xc);
}
// Restrict scroll history size to JBXVT_MAX_SCROLL:
static void trim(void)
{
	enum { SZ = sizeof(struct JBXVTLine), LIM = JBXVT_MAX_SCROLL };
	// Only work when scroll_size is twice JBXVT_MAX_SCROLL
	if (scroll_size < LIM << 1)
		return;
	struct JBXVTLine * new = malloc(LIM * SZ), * i;
	jb_require(new, "Cannot allocate memory for new scroll"
		" history buffer");
	const int diff = scroll_size - LIM;
	i = saved_lines + diff;
	memcpy(new, i, LIM * SZ);
	free(saved_lines);
	saved_lines = new;
	scroll_size = LIM;
}
static void add_scroll_history(void)
{
	struct JBXVTScreen * s = jbxvt_get_current_screen();
	enum { SIZE = sizeof(struct JBXVTLine) };
	saved_lines = realloc(saved_lines, ++scroll_size * SIZE);
	jb_require(saved_lines, "Cannot allocate memory"
		" for scroll history");
	// - 1 for index instead of size
	memcpy(&saved_lines[scroll_size - 1], &s->line[s->cursor.y], SIZE);
	trim();
}
static int8_t copy_lines(const int8_t i, const int8_t j, const int8_t mod,
	const int8_t count)
{
	if (i >= count)
		return j;
	struct JBXVTScreen * s = jbxvt_get_current_screen();
	struct JBXVTLine * dest = s->line + i,
			 * src = s->line + j;
	memcpy(dest, src, sizeof(struct JBXVTLine));
	return copy_lines(i + 1, j + mod, mod, count);
}
static void clear_line(xcb_connection_t * xc,
	const int16_t y, const int8_t count)
{
	const uint8_t fh = jbxvt_get_font_size().height;
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), 0, y * fh,
		jbxvt_get_pixel_size().width, count * fh);
}
static void clear(int8_t count, const uint8_t offset, const bool is_up)
{
	if (--count < 0)
		return;
	const uint8_t j = offset + (is_up ? - count - 1 : count);
	memset(jbxvt_get_current_screen()->line + j, 0,
		sizeof(struct JBXVTLine));
	clear(count, offset, is_up);
}
static void sc_common(xcb_connection_t * xc, const uint8_t r1, const
	uint8_t r2, const int16_t count, const bool up)
{
	if (up) // call this way to only have one branch
		clear(count, r2, true);
	else
		clear(count, r1, false);
	copy_visible_area(xc, r1, r2, count, up);
	clear_line(xc, up ? (r2 - count) : r1, count);
	jbxvt_draw_scrollbar(xc);
}
static void sc_dn(xcb_connection_t * xc, const uint8_t row1, const
	uint8_t row2, const int16_t count)
{
	struct JBXVTScreen * s = jbxvt_get_current_screen();
	for(int8_t j = copy_lines(0, row2, -1, count); j >= row1; --j)
		move_line(j, count, s);
	sc_common(xc, row1, row2, count, false);
}
static void sc_up(xcb_connection_t * xc, const uint8_t row1, const
	uint8_t row2, const int16_t count)
{
	struct JBXVTScreen * s = jbxvt_get_current_screen();
	if (s == jbxvt_get_screen_at(0) && row1 == 0)
		add_scroll_history();
	for(int8_t j = copy_lines(0, row1, 1, count); j < row2; ++j)
		move_line(j, -count, s);
	sc_common(xc, row1, row2, count, true);
}
/*  Scroll count lines from row1 to row2 inclusive.
    row1 should be <= row2.  Scrolling is up for a positive count and
    down for a negative count.  count is limited to a maximum of
    SCROLL lines.  */
void scroll(xcb_connection_t * xc, const uint8_t row1,
	const uint8_t row2, const int16_t count)
{
#if LOG_LEVEL > 5
	LOG("scroll(xc, row1: %d, row2: %d, count: %d)", row1, row2,
		count);
#endif//LOG_LEVEL>5
	if (JB_UNLIKELY(!count))
		return; // nothing to do
	if (JB_UNLIKELY(row1 > row2))
		return; // invalid
	(JB_LIKELY(count > 0) ? sc_up : sc_dn)(xc, row1, row2 + 1, abs(count));
	jbxvt_set_scroll(xc, 0);
#if LOG_LEVEL > 8
	for (uint16_t i = 0; i < scroll_size; ++i)
		puts((char *)saved_lines[i].text);
#endif//LOG_LEVEL>10
}
