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
	home_screen();
	struct screenst * s = jbxvt.scr.current;
	xcb_point_t c = s->cursor;
	if (c.x >= jbxvt.scr.chars.width - 1)
		  return;
	s->text[c.y][c.x] = ' ';
	while (++c.x % 8 && c.x < jbxvt.scr.chars.width - 1)
		  ;
}

static void handle_new_lines(int8_t nlcount)
{
	struct screenst * restrict s = jbxvt.scr.current;
	s->margin.b = MIN(s->margin.b, jbxvt.scr.chars.height - 1);
	if (s->cursor.y > s->margin.b)
		  nlcount = 0;
	else
		  nlcount -= s->margin.b - s->cursor.y;
	if (nlcount < 0)
		  nlcount = 0;
	else if (nlcount > (s->cursor.y - s->margin.t))
		  nlcount = s->cursor.y - s->margin.t;
	if (nlcount > MAX_SCROLL)
		  nlcount = MAX_SCROLL;
	scroll(s->margin.t, s->margin.b, nlcount);
	LOG("nlcount: %d, c.y: %d, m.b: %d", nlcount,
		s->cursor.y, s->margin.b);
	// This fixes ncdu scrolling:
	// FIXME: it also causes minor corrupt display.
#if 0
	if (s->cursor.y == s->margin.b - 2) {
		  scroll(s->margin.t + 2, s->margin.b - 2, 1);
	}
#endif
	s->cursor.y -= nlcount;
}

#if defined(__i386__) || defined(__amd64__)
       __attribute__((regparm(1)))
#endif//x86
static void handle_insert(const uint8_t n, const xcb_point_t p)
{
	SLOG("handle_insert(n=%d, p={%d, %d})", n, p.x, p.y);
	struct screenst * restrict c = jbxvt.scr.current;
	uint8_t * s = c->text [c->cursor.y];
	uint32_t * r = c->rend [c->cursor.y];
	memmove(s + c->cursor.x + n, s + c->cursor.x,
		jbxvt.scr.chars.width - c->cursor.x);
	memmove(r + c->cursor.x + n, r + c->cursor.x,
		(jbxvt.scr.chars.width - c->cursor.x) * sizeof(uint32_t));
	const uint16_t width = (jbxvt.scr.chars.width - c->cursor.x - n)
		* jbxvt.X.font_width;
	const int16_t x = p.x + n * jbxvt.X.font_width;
	xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, p.x, p.y, x, p.y, width, jbxvt.X.font_height);
}

static void handle_wrap_next(void)
{
	SLOG("handle_wrap_next()");
	struct screenst * restrict c = jbxvt.scr.current;
	c->text[c->cursor.y][jbxvt.scr.chars.width] = 1;
	if (c->cursor.y >= c->margin.bottom) {
		SLOG("cursor at bottom margin, scrolling");
		  scroll(c->margin.top, c->margin.bottom, 1);
	} else if (c->cursor.y < jbxvt.scr.chars.height - 1) {
		SLOG("++cursor.y");
		  ++c->cursor.y;
	}
	c->cursor.x = c->wrap_next = 0;
}

static void handle_line_feed(void)
{
	SLOG("handle_line_feed()");
	struct screenst * restrict c = jbxvt.scr.current;
	if (likely(c->cursor.y < jbxvt.scr.chars.height - 1)) {
		++c->cursor.y;
	} else if (c->cursor.y <= c->margin.bottom) {
		scroll(c->margin.top, c->margin.bottom,1);
	}
	check_selection(c->cursor.y, c->cursor.y);
	c->wrap_next = 0;
}

static void wrap_at_end(uint8_t * restrict str, const uint8_t len)
{
	struct screenst * restrict c = jbxvt.scr.current;
	if (unlikely(len > 0 && c->cursor.x == jbxvt.scr.chars.width
		&& *str >= ' ')) {
		SLOG("wrap_at_end(%s, %d)", str, len);
		c->text [c->cursor.y] [jbxvt.scr.chars.width] = 1;
		if (likely(c->cursor.y >= c->margin.bottom)) {
			scroll(c->margin.top, c ->margin.bottom, 1);
		} else {
			++c->cursor.y;
		}
		c->cursor.x = 0;
	}
}

// str and iter alias each other
static int_fast16_t find_n(uint8_t * str, uint8_t * iter)
{
	return *iter < ' ' ? iter - str : find_n(str, iter + 1);
}

/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void scr_string(uint8_t * restrict str, int8_t len, int8_t nlcount)
{
	SLOG("scr_string(%s, len: %d, nlcount: %d)", str, len, nlcount);
	home_screen();
	cursor(CURSOR_DRAW);
	if (nlcount > 0)
		  handle_new_lines(nlcount);
	xcb_point_t p;
	struct screenst * restrict c = jbxvt.scr.current;
	if (c->cursor.y < 0)
		  c->cursor.y = 0;
	if (c->cursor.y > jbxvt.scr.chars.height)
		  c->cursor.y = jbxvt.scr.chars.height;
	while (len) {
		if (likely(*str == '\r')) { // carriage return
			c->cursor.x = 0;
			c->wrap_next = 0;
			--len;
			++str;
			continue;
		} else if (*str == '\n') { // line feed
			handle_line_feed();
			--len;
			++str;
			continue;
		} else if (unlikely(*str == '\t')) {
			scr_tab();
			--len;
			++str;
			continue;
		}

		if (c->wrap_next)
			  handle_wrap_next();

		check_selection(c->cursor.y, c->cursor.y);
		p.x = MARGIN + jbxvt.X.font_width * c->cursor.x;
		p.y = MARGIN + jbxvt.X.font_height * c->cursor.y;
		const int_fast16_t n = find_n(str, str);
		if (unlikely(c->insert))
			  handle_insert(n, p);
		uint8_t * s = c->text[c->cursor.y];
		if (!s) return;
		s += c->cursor.x;
		if (c->charset[c->charsel] == CHARSET_SG0) {
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
		// Save scroll history:
		memcpy(s, str, n);
		// Render the string:
		paint_rval_text(str, jbxvt.scr.rstyle, n, p);
		// Save render style:
		if(jbxvt.scr.rstyle) {
			for (int_fast16_t i = n - 1; i >= 0; --i)
				  c->rend[c->cursor.y][c->cursor.x + i]
					  = jbxvt.scr.rstyle;
		}
		len -= n;
		str += n;
		c->cursor.x += n;
		wrap_at_end(str, len);
	}
	if (c->cursor.x >= jbxvt.scr.chars.width) {
		c->cursor.x = jbxvt.scr.chars.width - 1;
		c->wrap_next = c->decawm;
	}
	cursor(CURSOR_DRAW);
}


