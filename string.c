/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
//#undef DEBUG
//#define DEBUG_VERBOSE
#include "string.h"
#include <string.h>
#include "JBXVTLine.h"
#include "JBXVTPaintContext.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
#include "cursor.h"
#include "font.h"
#include "gc.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "libjb/time.h"
#include "mode.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "move.h"
#include "selection.h"
#include "size.h"
#include "tab.h"
#include "window.h"
static void handle_new_lines(xcb_connection_t * restrict xc,
	int8_t new_line_count)
{
	struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	const int16_t y = s->cursor.y;
	struct JBDim * m = &s->margin;
	new_line_count = y > m->b ? 0 : new_line_count - m->b - y;
	JB_LIMIT(new_line_count, y - m->top, 0);
	if (new_line_count) {
		// only worth doing if new_line_count has a value:
		scroll(xc, m->top, m->bottom, new_line_count);
		s->cursor.y -= new_line_count;
	}
}
static void decsclm(void)
{
	if (jbxvt_get_modes()->decsclm)
		jb_sleep(JBXVT_SOFT_SCROLL_DELAY);
}
static void wrap(xcb_connection_t * restrict xc)
{
	struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	s->wrap_next = false;
	const struct JBDim m = s->margin;
	const int16_t y = s->cursor.y;
	s->line[y].wrap = true;
	if (y >= m.b) {
		decsclm();
		scroll(xc, m.top, m.bottom, 1);
	} else
		++s->cursor.y;
}
struct MoveData {
	union {
		uint8_t * text;
		xcb_connection_t * xc;
	};
	union {
		rstyle_t * style;
		struct JBDim * restrict point;
	};
	size_t size;
	int32_t index, offset;
};
static void move_visible(struct MoveData * restrict d)

{
	const struct JBDim f = jbxvt_get_font_size(),
	      * restrict p = d->point;
	const uint16_t n_width = d->offset * f.width;
	xcb_connection_t * xc = d->xc;
	const xcb_window_t vt = jbxvt_get_vt_window(xc);
	xcb_copy_area(xc, vt, vt, jbxvt_get_text_gc(xc), p->x, p->y,
		p->x + n_width, p->y, d->size * f.width - n_width, f.height);
}
static void move(struct MoveData * restrict d)
{
	memmove(d->text + d->index + d->offset,
		d->text + d->index, d->size);
	memmove(d->style + d->index + d->offset,
		d->style + d->index, d->size << 2);
}
// Insert count characters space (not blanked) at point
static void insert_characters(xcb_connection_t * restrict xc,
	const uint8_t count, struct JBDim point)
{
	LOG("handle_insert(n=%d, p={%d, %d})", count, point.x, point.y);
	struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	const struct JBDim c = s->cursor;
	struct MoveData d = {.text = s->line[c.y].text,
		.style = s->line[c.y].rend, .offset = count,
		.index = c.x,
		.size = (size_t)(jbxvt_get_char_size().w - c.x)};
	move(&d);
	d.xc = xc;
	d.point = &point;
	move_visible(&d);
}
static void parse_special_charset(uint8_t * restrict str, const int i)
{
	LOG("parse_special_charset(str, %d)", i);
	if (i >= 0) {
		if (str[i] < 'q')
			str[i] = '+';
		else if (str[i] != 'x')
			str[i] = '-';
		else // x
			str[i] = '|';
		parse_special_charset(str, i - 1);
	}
}
static bool test_action_char(xcb_connection_t * xc, const uint8_t c,
	struct JBXVTScreen * restrict s)
{
	switch(c) {
	case '\r':
		s->cursor.x = 0;
		s->wrap_next = false;
		return true;
	case '\n':
		wrap(xc);
		return true;
	case '\t':
		jbxvt_tab(xc, 1);
		return true;
	}
	return false;
}
static void save_render_style(const int_fast16_t n,
	struct JBXVTScreen * restrict s)
{
	const struct JBDim c = s->cursor;
	const rstyle_t r = jbxvt_get_rstyle();
	for (int_fast16_t i = n - 1; i >= 0; --i)
		  s->line[c.y].rend[c.x + i] = r;
}
static void check_wrap(struct JBXVTScreen * restrict s)
{
	const uint16_t w = jbxvt_get_char_size().w;
	if (s->cursor.x >= w)
		s->wrap_next = !jbxvt_get_modes()->decawm;
}
static void clear_shift(struct JBXVTPrivateModes * restrict m)
{
	m->ss2 = false;
	m->ss3 = false;
}
static bool test_shift(struct JBXVTPrivateModes * restrict m,
	const bool shift, const enum JBXVTCharacterSet cs)
{
	bool rval = false; // nothing changed
	if (shift) {
		const uint8_t i = m->charsel; // current index
		m->charset[JBXVT_CHARSET_SHIFT_REGISTER]
			= m->charset[i]; // save in shift buffer
		m->charset[i] = cs; // set current to desired
		clear_shift(m);
		rval = true; // we have shifted
	}
	return rval;
}
static bool handle_single_shift(void)
{
	struct JBXVTPrivateModes * m = jbxvt_get_modes();
	bool rval = false; // nothing changed
	if (test_shift(m, m->ss2, CHARSET_SG2))
		rval = true; // we have shifted
	else if (test_shift(m, m->ss3, CHARSET_SG3))
		rval = true; // ditto
	return rval;
}
static void recover_from_shift(void)
{
	struct JBXVTPrivateModes * m = jbxvt_get_modes();
	m->charset[m->charsel] = m->charset[JBXVT_CHARSET_SHIFT_REGISTER];
}
struct DrawStringContext {
	xcb_connection_t * connection;
	struct JBXVTScreen * screen;
	uint8_t * string;
	int32_t length;
};
static void draw_string_at_pixel_position(struct DrawStringContext * restrict
	dc)
{
	struct JBXVTScreen * restrict s = dc->screen;
	const struct JBDim * restrict c = &s->cursor;
	const struct JBDim p = jbxvt_chars_to_pixels(*c);
	const struct JBXVTPrivateModes * restrict mode = jbxvt_get_modes();
	if (JB_UNLIKELY(mode->insert))
		insert_characters(dc->connection, 1, p);
	uint8_t * t = s->line[c->y].text + c->x;
	const bool shifted = handle_single_shift();
	if (shifted) {
		parse_special_charset(dc->string, 1);
		recover_from_shift();
	}
	if (mode->charset[mode->charsel] > CHARSET_ASCII) {
		if (shifted)
			parse_special_charset(dc->string + 1,
				dc->length - 1);
		else
			parse_special_charset(dc->string, dc->length);
	}
	// Render the string:
	if (!s->decpm) {
		jbxvt_paint(&(struct JBXVTPaintContext){.xc = dc->connection,
			.string = dc->string,
			.style = &(rstyle_t){jbxvt_get_rstyle()},
			.length = 1, .position = p,
			.is_double_width_line = s->line[c->y].dwl});
		// Save scroll history:
		*t = *dc->string;
	}

}
static void draw_string_at_cursor_position(struct DrawStringContext * restrict
	dsc)
{
	struct JBXVTScreen * restrict s = dsc->screen;
	struct JBDim * restrict c = &s->cursor;
	if (s->wrap_next) {
		wrap(dsc->connection);
		c->x = 0;
	}
	jbxvt_check_selection(dsc->connection, c->y, c->y);
	draw_string_at_pixel_position(dsc);
	/* save_render_style() depends on the current cursor
	 * position, so it must be called before increment. */
	save_render_style(1, s);
	++c->x;
}
static void draw_next_char(struct DrawStringContext * restrict dsc)
{
	if (dsc->length > 0) {
		if (!test_action_char(dsc->connection, *dsc->string,
			dsc->screen)) {
			draw_string_at_cursor_position(dsc);
			check_wrap(dsc->screen);
		}
		dsc->string++;
		dsc->length--;
		draw_next_char(dsc);
	}
}
/*  Display the string at the current position.
    new_line_count is the number of new lines in the string.  */
void jbxvt_string(xcb_connection_t * xc, uint8_t * restrict str, int len,
	const int new_line_count)
{
#ifdef DEBUG_VERBOSE
	LOG("jbxvt_string(%s, len: %lu, new_line_count: %d)", str, len,
		new_line_count);
#endif//DEBUG_VERBOSE
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc);
	if (new_line_count > 0)
		  handle_new_lines(xc, new_line_count);
	jbxvt_check_cursor_position();
	struct DrawStringContext dsc = {.connection = xc, .screen =
		jbxvt_get_current_screen(), .string = str, .length = len};
	draw_next_char(&dsc);
	jbxvt_draw_cursor(xc);
}
