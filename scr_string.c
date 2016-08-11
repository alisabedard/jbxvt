/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "scr_string.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_move.h"
#include "selection.h"

#include <string.h>
#include <unistd.h>

//#define STRING_DEBUG
#ifdef STRING_DEBUG
#define SLOG(...) LOG(__VA_ARGS__)
#else
#define SLOG(...)
#endif

static bool tab_stops[JBXVT_MAX_COLS];

// Set tab stops:
// -1 clears all, -2 sets default
void scr_set_tab(int i, const bool value)
{
	if (i == -1) // clear all
		memset(&tab_stops, 0, JBXVT_MAX_COLS);
	else if (i == -2) // establish tab stop every 8 columns
		for (i = 0; i < JBXVT_MAX_COLS; ++i)
			tab_stops[i] = (i % 8 == 0);
	else if (i >= 0) // assign
		tab_stops[i] = value;
}

//  Tab to the next tab_stop.
void scr_tab(void)
{
	LOG("scr_tab()");
	change_offset(0);
	VTScreen * s = jbxvt.scr.current;
	const uint8_t w = jbxvt.scr.chars.width - 1;
	xcb_point_t c = s->cursor;
	s->text[c.y][c.x] = '0';
	while (!tab_stops[++c.x] && c.x < w);
	s->cursor.x = c.x;
}

void scr_cht(int16_t v)
{
	while (v-- > 0)
		scr_tab();
}

static void handle_new_lines(int8_t nlcount)
{
	SLOG("handle_new_lines(nlcount: %d)", nlcount);
	VTScreen * restrict s = jbxvt.scr.current;
	nlcount = s->cursor.y > s->margin.b ? 0
		: nlcount - s->margin.b - s->cursor.y;
	nlcount = MAX(nlcount, 0);
	const int8_t lim = s->cursor.y - s->margin.t;
	nlcount = MIN(nlcount, lim);
	nlcount = MIN(nlcount, MAX_SCROLL);
	scroll(s->margin.t, s->margin.b, nlcount);
	s->cursor.y -= nlcount;
	SLOG("nlcount: %d, c.y: %d, m.b: %d", nlcount,
		s->cursor.y, s->margin.b);
}

static void decsclm(void)
{
	if (!jbxvt.mode.decsclm)
		return;
	LOG("slow scroll");
	// Time value per the following:
	// http://www.vt100.net/docs/vt100-ug/chapter3.html166666
	usleep(166666);
}

static void wrap(void)
{
	VTScreen * s = jbxvt.scr.current;
	s->wrap_next = false;
	const Size m = s->margin;
	int16_t * y = &s->cursor.y;
	s->wrap[*y] = true;
	if (*y >= m.b) {
		decsclm();
		scroll(m.top, m.bottom, 1);
	} else
		++*y;
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
	const uint16_t sz = ch.width - cur.x;
	memmove(s + cur.x + n, s + cur.x, sz);
	memmove(r + cur.x + n, r + cur.x, sz << 2);
	const Size f = jbxvt.X.f.size;
	const uint16_t width = (sz - n) * f.w;
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
	const Size f = jbxvt.X.f.size;
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
		wrap();
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

static void check_wrap(VTScreen * restrict s)
{
	const uint8_t w = jbxvt.scr.chars.width - 1;
	if (s->cursor.x >= w) {
		s->cursor.x = w;
		s->wrap_next = jbxvt.mode.decawm;
	}
}

/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void scr_string(uint8_t * restrict str, uint8_t len, int8_t nlcount)
{
	SLOG("scr_string(%s, len: %d, nlcount: %d)", str, len, nlcount);
	change_offset(0);
	draw_cursor();
	if (nlcount > 0)
		  handle_new_lines(nlcount);
	xcb_point_t p;
	VTScreen * restrict s = jbxvt.scr.current;
	fix_cursor(&jbxvt.scr.s[0]);
	fix_cursor(&jbxvt.scr.s[1]);
	while (len) {
		if (test_action_char(*str, s)) {
			--len;
			++str;
			continue;
		} else if (unlikely(*str == 0xe2))
			*str = '+';
		if (s->wrap_next) {
			wrap();
			s->cursor.x = 0;
		}
		check_selection(s->cursor.y, s->cursor.y);
		p = get_p(s);
		const int_fast16_t n = find_n(str);
		if (unlikely(jbxvt.mode.insert))
			handle_insert(n, p);
		uint8_t * t = s->text[s->cursor.y];
		if (!t) return;
		t += s->cursor.x;
		if (jbxvt.mode.charset[jbxvt.mode.charsel] == CHARSET_SG0)
			parse_special_charset(str, len);
		// Render the string:
		if (!s->decpm) {
			paint_rval_text(str, jbxvt.scr.rstyle, n, p);
			// Save scroll history:
			memcpy(t, str, n);
		}
		save_render_style(n, s);
		len -= n;
		str += n;
		s->cursor.x += n;
		check_wrap(s);
	}
	draw_cursor();
}


