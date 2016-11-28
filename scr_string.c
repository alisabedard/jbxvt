/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "scr_string.h"
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "cursor.h"
#include "font.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "libjb/time.h"
#include "libjb/util.h"
#include "mode.h"
#include "paint.h"
#include "repaint.h"
#include "rstyle.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_move.h"
#include "selection.h"
#include "size.h"
#include "tab.h"
#include "window.h"
//#define STRING_DEBUG
#ifndef STRING_DEBUG
#undef LOG
#define LOG(...)
#endif//!STRING_DEBUG
static void handle_new_lines(xcb_connection_t * restrict xc, int8_t nlcount)
{
	struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	const int16_t y = s->cursor.y;
	struct JBDim * m = &s->margin;
	nlcount = y > m->b ? 0 : nlcount - m->b - y;
	JB_LIMIT(nlcount, y - m->top, 0);
	scroll(xc, m->top, m->bottom, nlcount);
	s->cursor.y -= nlcount;
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
	s->wrap[y] = true;
	if (y >= m.b) {
		decsclm();
		scroll(xc, m.top, m.bottom, 1);
	} else
		++s->cursor.y;
}
static void move_data_in_memory(uint8_t * restrict t,
	uint32_t * restrict r, const uint8_t n, const int16_t x,
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
static void handle_insert(xcb_connection_t * restrict xc,
	const uint8_t n, const struct JBDim p)
{
	LOG("handle_insert(n=%d, p={%d, %d})", n, p.x, p.y);
	const struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	const struct JBDim c = s->cursor;
	const uint16_t sz = jbxvt_get_char_size().w - c.x;
	move_data_in_memory(s->text[c.y], s->rend[c.y], n, c.x, sz);
	move_data_on_screen(xc, n, sz, p);
}
static void parse_special_charset(uint8_t * restrict str,
	const uint8_t len)
{
	LOG("CHARSET_SG0");
	for (int_fast16_t i = len ; i >= 0; --i) {
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
		jbxvt_tab(xc);
		return true;
	}
	return false;
}
static void save_render_style(const int_fast16_t n,
	struct JBXVTScreen * restrict s)
{
	const struct JBDim c = s->cursor;
	const uint32_t r = jbxvt_get_rstyle();
	for (int_fast16_t i = n - 1; i >= 0; --i)
		  s->rend[c.y][c.x + i] = r;
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
		m->charset[2] = m->charset[i]; // save in shift buffer
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
	m->charset[m->charsel] = m->charset[2];
}
/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void jbxvt_string(xcb_connection_t * xc, uint8_t * restrict str,
	uint8_t len, int8_t nlcount)
{
	LOG("jbxvt_string(%s, len: %d, nlcount: %d)", str, len, nlcount);
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc);
	if (nlcount > 0)
		  handle_new_lines(xc, nlcount);
	struct JBXVTScreen * restrict screen = jbxvt_get_current_screen();
	jbxvt_check_cursor_position();
	while (len) {
		if (test_action_char(xc, *str, screen)) {
			--len;
			++str;
			continue;
		}
		struct JBDim * c = &screen->cursor;
		if (screen->wrap_next) {
			wrap(xc);
			c->x = 0;
		}
		jbxvt_check_selection(xc, c->y, c->y);
		{
			struct JBDim p
				= jbxvt_chars_to_pixels(screen->cursor);
			struct JBXVTPrivateModes * restrict mode
				= jbxvt_get_modes();
			if (JB_UNLIKELY(mode->insert))
				handle_insert(xc, 1, p);
			uint8_t * t = screen->text[c->y] + c->x;
			const bool shifted = handle_single_shift();
			if (shifted) {
				parse_special_charset(str, 1);
				recover_from_shift();
			}
			if (mode->charset[mode->charsel] > CHARSET_ASCII) {
				if (shifted)
					parse_special_charset(str + 1,
						len - 1);
				else
					parse_special_charset(str, len);
			}
			// Render the string:
			if (!screen->decpm) {
				jbxvt_paint(xc, str, jbxvt_get_rstyle(),
					1, p, screen->dwl[c->y]);
				// Save scroll history:
				*t = *str;
			}
		}
		save_render_style(1, screen);
		--len;
		++str;
		++c->x;
		check_wrap(screen);
	}
	jbxvt_draw_cursor(xc);
}
