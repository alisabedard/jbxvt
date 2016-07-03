/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_string.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "screen.h"
#include "scroll.h"
#include "scr_move.h"
#include "selection.h"

#include <string.h>

//#define STRING_DEBUG
#ifdef STRING_DEBUG
#define SLOG(...) LOG(__VA_ARGS__)
#else
#define SLOG(...)
#endif

//  Tab to the next tab_stop.
void scr_tab(void)
{
	LOG("scr_tab()");
	home_screen();
	VTScreen * s = jbxvt.scr.current;
	const uint8_t w = jbxvt.scr.chars.width - 1;
	xcb_point_t c = s->cursor;
	s->text[c.y][c.x] = '0';
	while (++c.x % 8 && c.x < w);
	s->cursor.x = c.x;
}

static void handle_new_lines(int8_t nlcount)
{
	LOG("handle_new_lines(nlcount: %d)", nlcount);
	VTScreen * restrict s = jbxvt.scr.current;
	nlcount = s->cursor.y > s->margin.b ? 0
		: nlcount - s->margin.b - s->cursor.y;
	nlcount = MAX(nlcount, 0);
	const int8_t lim = s->cursor.y - s->margin.t;
	nlcount = MIN(nlcount, lim);
	nlcount = MIN(nlcount, MAX_SCROLL);
	scroll(s->margin.t, s->margin.b, nlcount);
	s->cursor.y -= nlcount;
	LOG("nlcount: %d, c.y: %d, m.b: %d", nlcount,
		s->cursor.y, s->margin.b);
}

static void wrap(VTScreen * restrict c)
{
	int16_t * y = &c->cursor.y;
	const Size sz = jbxvt.scr.chars;
	c->text[*y][sz.w] = 1; // wrap flag
	const Size m = c->margin;
	if (*y >= m.bottom) {
		LOG("cursor at bottom margin, scrolling");
		scroll(m.top, m.bottom, 1);
	} else if (*y < sz.height - 1) {
		SLOG("++*y");
		++*y;
	}
	check_selection(*y, *y);
	c->wrap_next = false;
}

#if defined(__i386__) || defined(__amd64__)
       __attribute__((regparm(2)))
#endif//x86
static void handle_insert(const uint8_t n, const xcb_point_t p)
{
	SLOG("handle_insert(n=%d, p={%d, %d})", n, p.x, p.y);
	VTScreen * restrict c = jbxvt.scr.current;
	uint8_t * s = c->text [c->cursor.y];
	uint32_t * r = c->rend [c->cursor.y];
	const Size ch = jbxvt.scr.chars;
	const xcb_point_t cur = c->cursor;
	memmove(s + cur.x + n, s + cur.x, ch.width - cur.x);
	memmove(r + cur.x + n, r + cur.x,
		(ch.width - cur.x) * sizeof(uint32_t));
	const Size f = jbxvt.X.font_size;
	const uint16_t width = (ch.width - cur.x - n)
		* f.w;
	const int16_t x = p.x + n * f.w;
	xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, p.x, p.y, x, p.y, width, f.h);
}

static uint_fast16_t find_n(uint8_t * restrict str)
{
	uint_fast16_t i;
	for (i = 0; i < jbxvt.scr.chars.width && str[i] >= ' '; ++i)
		  ;
	return i;
}

static void parse_special_charset(uint8_t * restrict str, const uint8_t len)
{
	SLOG("CHARSET_SG0");
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

static inline xcb_point_t get_p(VTScreen * restrict c)
{
	const Size f = jbxvt.X.font_size;
	return (xcb_point_t){.x = MARGIN + f.w * c->cursor.x,
		.y = MARGIN + f.h * c->cursor.y};
}

static void fix_margins(Size * restrict m, const int16_t cursor_y)
{
	m->b = MAX(m->b, cursor_y);
	const uint8_t h = jbxvt.scr.chars.height - 1;
	m->b = MIN(m->b, h);
}

static void fix_cursor(VTScreen * restrict c)
{
	c->cursor.y = MAX(c->cursor.y, 0);
	c->cursor.y = MIN(c->cursor.y, jbxvt.scr.chars.height - 1);
	c->cursor.x = MAX(c->cursor.x, 0);
	c->cursor.x = MIN(c->cursor.x, jbxvt.scr.chars.width - 1);
	fix_margins(&c->margin, c->cursor.y);
}

static bool test_action_char(const uint8_t c, VTScreen * restrict s)
{
	switch(c) {
	case '\r':
		s->cursor.x = 0;
		s->wrap_next = false;
		return true;
	case '\n':
		wrap(s);
		return true;
	case '\t':
		scr_tab();
		return true;
	}
	return false;
}

static void save_render_style(const int_fast16_t n, VTScreen * restrict s)
{
	for (int_fast16_t i = n - 1; i >= 0; --i)
		  s->rend[s->cursor.y][s->cursor.x + i] = jbxvt.scr.rstyle;
}

/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void scr_string(uint8_t * restrict str, uint8_t len, int8_t nlcount)
{
	SLOG("scr_string(%s, len: %d, nlcount: %d)", str, len, nlcount);
	home_screen();
	cursor(CURSOR_DRAW);
	if (nlcount > 0)
		  handle_new_lines(nlcount);
	xcb_point_t p;
	VTScreen * restrict s = jbxvt.scr.current;
	fix_cursor(&jbxvt.scr.s1);
	fix_cursor(&jbxvt.scr.s2);
	while (len) {
		if (test_action_char(*str, s)) {
			--len; ++str; continue;
		} else if (unlikely(*str == 0xe2))
			  *str = '+';
		if (s->wrap_next) {
			wrap(s);
			s->cursor.x = 0;
		}
		check_selection(s->cursor.y, s->cursor.y);
		p = get_p(s);
		const int_fast16_t n = find_n(str);
		if (unlikely(s->insert))
			  handle_insert(n, p);
		uint8_t * t = s->text[s->cursor.y];
		if (!t) return;
		t += s->cursor.x;
		if (s->charset[s->charsel] == CHARSET_SG0)
			  parse_special_charset(str, len);
		// Render the string:
		paint_rval_text(str, jbxvt.scr.rstyle, n, p);
		// Save scroll history:
		memcpy(t, str, n);
		// Save render style:
		if(jbxvt.scr.rstyle)
			  save_render_style(n, s);
		len -= n;
		str += n;
		s->cursor.x += n;
		const uint8_t w = jbxvt.scr.chars.width;
		if (s->cursor.x >= w) {
			s->cursor.x = jbxvt.scr.chars.width - 1;
			if (!(s->wrap_next = s->decawm))
				  break; /* Skip rendering off the edge of the
					    screen.  */
		}
	}
	cursor(CURSOR_DRAW);
}


