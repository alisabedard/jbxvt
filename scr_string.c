/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "scr_string.h"
#include "config.h"
#include "cursor.h"
#include "font.h"
#include "jbxvt.h"
#include "libjb/log.h"
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
#include <string.h>
#include <unistd.h>
//#define STRING_DEBUG
#ifndef STRING_DEBUG
#undef LOG
#define LOG(...)
#endif//!STRING_DEBUG
static void handle_new_lines(xcb_connection_t * xc, int8_t nlcount)
{
	const int16_t y = jbxvt_get_screen()->cursor.y;
	struct JBDim * m = &jbxvt_get_screen()->margin;
	nlcount = y > m->b ? 0 : nlcount - m->b - y;
	JB_LIMIT(nlcount, y - m->top, 0);
	scroll(xc, m->top, m->bottom, nlcount);
	jbxvt_get_screen()->cursor.y -= nlcount;
}
static void decsclm(void)
{
	// Time value per the following:
	// http://www.vt100.net/docs/vt100-ug/chapter3.html166666
	if (jbxvt_get_modes()->decsclm)
		jb_sleep(166);
}
static void wrap(xcb_connection_t * xc)
{
	jbxvt_get_screen()->wrap_next = false;
	const struct JBDim m = jbxvt_get_screen()->margin;
	const int16_t y = jbxvt_get_screen()->cursor.y;
	jbxvt_get_screen()->wrap[y] = true;
	if (y >= m.b) {
		decsclm();
		scroll(xc, m.top, m.bottom, 1);
	} else
		++jbxvt_get_screen()->cursor.y;
}
#if defined(__i386__) || defined(__amd64__)
       __attribute__((regparm(2)))
#endif//x86
static void handle_insert(xcb_connection_t * xc,
	const uint8_t n, const struct JBDim p)
{
	LOG("handle_insert(n=%d, p={%d, %d})", n, p.x, p.y);
	const struct JBDim c = jbxvt_get_screen()->cursor;
	uint8_t * restrict s = jbxvt_get_screen()->text[c.y];
	uint32_t * restrict r = jbxvt_get_screen()->rend[c.y];
	const uint16_t sz = jbxvt_get_char_size().w - c.x;
	memmove(s + c.x + n, s + c.x, sz);
	memmove(r + c.x + n, r + c.x, sz << 2);
	const struct JBDim f = jbxvt_get_font_size();
	const uint16_t n_width = n * f.width;
	const uint16_t width = sz * f.width - n_width;
	const int16_t x = p.x + n_width;
	xcb_copy_area(xc, jbxvt_get_vt_window(xc), jbxvt_get_vt_window(xc),
		jbxvt_get_text_gc(xc), p.x, p.y, x, p.y, width, f.height);
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
static void fix_margins(struct JBDim* restrict m,
	const int16_t cursor_y)
{
	m->b = JB_MAX(m->b, cursor_y);
	const uint8_t h = jbxvt_get_char_size().height - 1;
	m->b = JB_MIN(m->b, h);
}
static void fix_cursor(struct JBXVTScreen * restrict c)
{
	JB_LIMIT(c->cursor.y, jbxvt_get_char_size().height - 1, 0);
	JB_LIMIT(c->cursor.x, jbxvt_get_char_size().width - 1, 0);
	fix_margins(&c->margin, c->cursor.y);
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
/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void jbxvt_string(xcb_connection_t * xc,
	uint8_t * restrict str, uint8_t len, int8_t nlcount)
{
	LOG("jbxvt_string(%s, len: %d, nlcount: %d)", str, len, nlcount);
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc);
	if (nlcount > 0)
		  handle_new_lines(xc, nlcount);
	struct JBDim p;
	fix_cursor(jbxvt_get_screen_at(0));
	fix_cursor(jbxvt_get_screen_at(1));
	while (len) {
		if (test_action_char(xc, *str, jbxvt_get_screen())) {
			--len;
			++str;
			continue;
		}
		struct JBDim * c = &jbxvt_get_screen()->cursor;
		if (jbxvt_get_screen()->wrap_next) {
			wrap(xc);
			c->x = 0;
		}
		jbxvt_check_selection(xc, c->y, c->y);
		p = jbxvt_chars_to_pixels(jbxvt_get_screen()->cursor);
		if (JB_UNLIKELY(jbxvt_get_modes()->insert))
			handle_insert(xc, 1, p);
		uint8_t * t = jbxvt_get_screen()->text[c->y];
		if (!t) // should never be NULL.
			abort();
		t += c->x;
		if (jbxvt_get_modes()->charset[jbxvt_get_modes()
			->charsel] > CHARSET_ASCII)
			parse_special_charset(str, len);
		// Render the string:
		if (!jbxvt_get_screen()->decpm) {
			jbxvt_paint(xc, str, jbxvt_get_rstyle(), 1, p,
				jbxvt_get_screen()->dwl[c->y]);
			// Save scroll history:
			*t = *str;
		}
		save_render_style(1, jbxvt_get_screen());
		--len;
		++str;
		++c->x;
		check_wrap(jbxvt_get_screen());
	}
	jbxvt_draw_cursor(xc);
}
