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

#define CUR SCR->cursor
#define VT jbxvt.X.win.vt

// Set tab stops:
// -1 clears all, -2 sets default
void scr_set_tab(int16_t i, const bool value)
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
	struct JBDim c = SCR->cursor;
	SCR->text[c.y][c.x] = ' ';
	const uint16_t w = CSZ.w - 1;
	while (!tab_stops[++c.x] && c.x < w);
	SCR->cursor.x = c.x;
}

void scr_cht(int16_t v)
{
	while (v-- > 0)
		scr_tab();
}

static void handle_new_lines(int8_t nlcount)
{
	SLOG("handle_new_lines(nlcount: %d)", nlcount);
	const uint8_t b = SCR->margin.b;
	const uint8_t y = SCR->cursor.y;
	nlcount = y > b ? 0 : nlcount - b - y;
	const uint8_t t = SCR->margin.t;
	const int8_t lim = y - t;
	JB_LIMIT(nlcount, lim, 0);
	nlcount = MIN(nlcount, MAX_SCROLL);
	scroll(t, b, nlcount);
	SCR->cursor.y -= nlcount;
	SLOG("nlcount: %d, c.y: %d, m.b: %d", nlcount, SCR->cursor.y, b);
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
	SCR->wrap_next = false;
	const struct JBDim m = SCR->margin;
	int16_t * y = &SCR->cursor.y;
	SCR->wrap[*y] = true;
	if (*y >= m.b) {
		decsclm();
		scroll(m.top, m.bottom, 1);
	} else
		++*y;
}

#if defined(__i386__) || defined(__amd64__)
       __attribute__((regparm(2)))
#endif//x86
static void handle_insert(const uint8_t n, const struct JBDim p)
{
	SLOG("handle_insert(n=%d, p={%d, %d})", n, p.x, p.y);
	const struct JBDim c = SCR->cursor;
	uint8_t * restrict s = SCR->text[c.y];
	uint32_t * restrict r = SCR->rend[c.y];
	const uint16_t sz = CSZ.w - c.x;
	memmove(s + c.x + n, s + c.x, sz);
	memmove(r + c.x + n, r + c.x, sz << 2);
	const uint16_t n_width = n * FSZ.width;
	const uint16_t width = sz * FSZ.width - n_width;
	const int16_t x = p.x + n_width;
	xcb_copy_area(jbxvt.X.xcb, VT, VT, jbxvt.X.gc.tx, p.x, p.y,
		x, p.y, width, FSZ.height);
}

static void parse_special_charset(uint8_t * restrict str,
	const uint8_t len)
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

static void fix_margins(struct JBDim* restrict m,
	const int16_t cursor_y)
{
	m->b = MAX(m->b, cursor_y);
	const uint8_t h = jbxvt.scr.chars.height - 1;
	m->b = MIN(m->b, h);
}

static void fix_cursor(struct JBXVTScreen * restrict c)
{
	JB_LIMIT(c->cursor.y, jbxvt.scr.chars.height - 1, 0);
	JB_LIMIT(c->cursor.x, jbxvt.scr.chars.width - 1, 0);
	fix_margins(&c->margin, c->cursor.y);
}

static bool test_action_char(const uint8_t c,
	struct JBXVTScreen * restrict s)
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

static void save_render_style(const int_fast16_t n,
	struct JBXVTScreen * restrict s)
{
	const struct JBDim c = s->cursor;
	for (int_fast16_t i = n - 1; i >= 0; --i)
		  s->rend[c.y][c.x + i] = jbxvt.scr.rstyle;
}

static void check_wrap(struct JBXVTScreen * restrict s)
{
	const uint16_t w = CSZ.w;
	if (s->cursor.x >= w)
		s->wrap_next = !jbxvt.mode.decawm;
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
	struct JBDim p;
	fix_cursor(&jbxvt.scr.s[0]);
	fix_cursor(&jbxvt.scr.s[1]);
	while (len) {
		if (test_action_char(*str, SCR)) {
			--len;
			++str;
			continue;
		}
		if (SCR->wrap_next) {
			wrap();
			SCR->cursor.x = 0;
		}
		check_selection(SCR->cursor.y, SCR->cursor.y);
		p = get_p(SCR->cursor);
		if (unlikely(jbxvt.mode.insert))
			handle_insert(1, p);
		uint8_t * t = SCR->text[SCR->cursor.y];
		if (!t) return;
		t += SCR->cursor.x;
		if (jbxvt.mode.charset[jbxvt.mode.charsel] > CHARSET_ASCII)
			parse_special_charset(str, len);
		// Render the string:
		if (!SCR->decpm) {
			paint_rval_text(str, jbxvt.scr.rstyle, 1, p);
			// Save scroll history:
			*t = *str;
		}
		save_render_style(1, SCR);
		--len;
		++str;
		++SCR->cursor.x;
		check_wrap(SCR);
	}
	draw_cursor();
}


