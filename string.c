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
	// Time value per the following:
	// http://www.vt100.net/docs/vt100-ug/chapter3.html166666
	if (jbxvt_get_modes()->decsclm)
		jb_sleep(166);
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
static void move_data_in_memory(uint8_t * restrict t,
	rstyle_t * restrict r, const uint8_t n, const int16_t x,
	const uint16_t sz)
{
	memmove(t + x + n, t + x, sz);
	memmove(r + x + n, r + x, sz << 2);
}
static void move_data_on_screen(xcb_connection_t * xc,
	const uint8_t n, const uint16_t sz, const struct JBDim p)
{
	const struct JBDim f = jbxvt_get_font_size();
	const uint16_t n_width = n * f.width;
	const xcb_window_t vt = jbxvt_get_vt_window(xc);
	xcb_copy_area(xc, vt, vt, jbxvt_get_text_gc(xc), p.x, p.y,
		p.x + n_width, p.y, sz * f.width - n_width, f.height);

}
// Insert count characters space (not blanked) at point
static void insert_characters(xcb_connection_t * restrict xc,
	const uint8_t count, const struct JBDim point)
{
	LOG("handle_insert(n=%d, p={%d, %d})", count, point.x, point.y);
	struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	const struct JBDim c = s->cursor;
	const uint16_t sz = jbxvt_get_char_size().w - c.x;
	move_data_in_memory(s->line[c.y].text, s->line[c.y].rend,
		count, c.x, sz);
	move_data_on_screen(xc, count, sz, point);
}
static void parse_special_charset(uint8_t * restrict str,
	const int len)
{
	for (int i = len; i >= 0; --i) {
		uint8_t * ch = &str[i];
		switch (*ch) {
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 't':
		case 'u':
			*ch = '+';
			break;
		case 'q':
			*ch = '-';
			break;
		case 'x':
			*ch = '|';
			break;
		}
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
	if (shift) {
		const uint8_t i = m->charsel; // current index
		m->charset[JBXVT_CHARSET_SHIFT_REGISTER]
			= m->charset[i]; // save in shift buffer
		m->charset[i] = cs; // set current to desired
		clear_shift(m);
		return true; // we have shifted
	}
	return false; // nothing changed
}
static bool handle_single_shift(void)
{
	struct JBXVTPrivateModes * m = jbxvt_get_modes();
	if (test_shift(m, m->ss2, CHARSET_SG2))
		return true; // we have shifted
	if (test_shift(m, m->ss3, CHARSET_SG3))
		return true; // ditto
	return false; // nothing changed
}
static void recover_from_shift(void)
{
	struct JBXVTPrivateModes * m = jbxvt_get_modes();
	m->charset[m->charsel] = m->charset[JBXVT_CHARSET_SHIFT_REGISTER];
}
static void draw_string_at_pixel_position(xcb_connection_t * xc,
	struct JBXVTScreen * screen, uint8_t * restrict str,
	const int len)
{
	struct JBDim p
		= jbxvt_chars_to_pixels(screen
			->cursor);
	struct JBXVTPrivateModes * restrict mode
		= jbxvt_get_modes();
	if (JB_UNLIKELY(mode->insert))
		insert_characters(xc, 1, p);
	struct JBDim * c = &screen->cursor;
	uint8_t * t = screen->line[c->y].text + c->x;
	const bool shifted = handle_single_shift();
	if (shifted) {
		parse_special_charset(str, 1);
		recover_from_shift();
	}
	if (mode->charset[mode->charsel]
		> CHARSET_ASCII) {
		if (shifted)
			parse_special_charset(str + 1,
				len - 1);
		else
			parse_special_charset(str,
				len);
	}
	// Render the string:
	if (!screen->decpm) {
		jbxvt_paint(&(struct
			JBXVTPaintContext){.xc = xc,
			.string = str, .style =
			&(rstyle_t){
				jbxvt_get_rstyle()},
			.length = 1, .position = p,
			.is_double_width_line =
			screen->line[c->y].dwl});
		// Save scroll history:
		*t = *str;
	}

}
static void draw_string_at_cursor_position(xcb_connection_t * xc,
	struct JBXVTScreen * screen, uint8_t * restrict str,
	const int len)
{
	struct JBDim * c = &screen->cursor;
	if (screen->wrap_next) {
		wrap(xc);
		c->x = 0;
	}
	jbxvt_check_selection(xc, c->y, c->y);
	draw_string_at_pixel_position(xc, screen, str, len);
	/* save_render_style() depends on the current cursor
	 * position, so it must be called before increment. */
	save_render_style(1, screen);
	++c->x;
}
/*  Display the string at the current position.
    new_line_count is the number of new lines in the string.  */
void jbxvt_string(xcb_connection_t * xc, uint8_t * restrict str, uint_fast16_t
	len, const int8_t new_line_count)
{
#ifdef DEBUG_VERBOSE
	LOG("jbxvt_string(%s, len: %lu, new_line_count: %d)", str, len,
		new_line_count);
#endif//DEBUG_VERBOSE
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc);
	if (new_line_count > 0)
		  handle_new_lines(xc, new_line_count);
	struct JBXVTScreen * restrict screen = jbxvt_get_current_screen();
	jbxvt_check_cursor_position();
	while (len) {
		if (test_action_char(xc, *str, screen)) {
			--len;
			++str;
			continue;
		}
		draw_string_at_cursor_position(xc,screen, str, len);
		++str;
		--len;
		check_wrap(screen);
	}
	jbxvt_draw_cursor(xc);
}
