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

static void wrap(VTScreen * restrict c)
{
	const Size m = c->margin;
	const Size sz = jbxvt.scr.chars;
	int16_t * y = &c->cursor.y;
	c->text[*y][sz.w] = 1; // wrap flag
	if (*y >= m.bottom) {
		LOG("cursor at bottom margin, scrolling");
		scroll(m.top, m.bottom, 1);
	} else if (*y < sz.height - 1) {
		SLOG("++*y");
		++*y;
	}
	check_selection(*y, *y);
	c->wrap_next = 0;
	c->cursor.x = 0;
}

static void handle_new_lines(int8_t nlcount)
{
	LOG("handle_new_lines(nlcount: %d)", nlcount);
	VTScreen * restrict s = jbxvt.scr.current;
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
	s->cursor.y -= nlcount;
	LOG("nlcount: %d, c.y: %d, m.b: %d", nlcount,
		s->cursor.y, s->margin.b);
}

#if defined(__i386__) || defined(__amd64__)
       __attribute__((regparm(1)))
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

static void fix_margins(VTScreen * restrict s)
{
	const uint8_t h = jbxvt.scr.chars.height - 1;
	s->margin.b = MAX(s->margin.b, s->cursor.y);
	s->margin.b = MIN(s->margin.b, h);
}

static inline void fix_cursor(VTScreen * restrict c)
{
	c->cursor.y = MAX(c->cursor.y, 0);
	c->cursor.y = MIN(c->cursor.y, jbxvt.scr.chars.height - 1);
	c->cursor.x = MAX(c->cursor.x, 0);
	c->cursor.x = MIN(c->cursor.x, jbxvt.scr.chars.width - 1);
	fix_margins(c);
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
#define NXT_CHR() --len; ++str; continue;
		if (likely(*str == '\r')) { // carriage return
			s->cursor.x = 0;
			s->wrap_next = 0;
			NXT_CHR();
		} else if (*str == '\n') { // line feed
			wrap(s);
			NXT_CHR();
		} else if (unlikely(*str == '\t')) {
			scr_tab();
			NXT_CHR();
		} else if (unlikely(*str == 0xe2)) {
			*str = '+';
		} // mask out tmux graphics characters
		if (s->wrap_next)
			wrap(s);
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
		// Save scroll history:
		//memcpy(t, str, n);
		strncpy((char*)t, (char*)str, n);
		// Render the string:
		paint_rval_text(str, jbxvt.scr.rstyle, n, p);
		// Save render style:
		if(jbxvt.scr.rstyle) {
			for (int_fast16_t i = n - 1; i >= 0; --i)
				  s->rend[s->cursor.y][s->cursor.x + i]
					  = jbxvt.scr.rstyle;
		}
		len -= n;
		str += n;
		s->cursor.x += n;
		const uint8_t w = jbxvt.scr.chars.width;
		if (s->cursor.x >= w) {
			s->cursor.x = jbxvt.scr.chars.width - 1;
			s->wrap_next = s->decawm;
		}
#if 0
			if (s->decawm)
				//wrap(s);
				s->wrap_next = true;
			else {
				s->cursor.x = w - 1;
				s->wrap_next = false;
				cursor(CURSOR_DRAW);
				return;
			}
#endif
	//	}
#if 0
		if (unlikely(len > 0 && s->cursor.x
			== jbxvt.scr.chars.width && *str >= ' ')) {
			// Handle DEC auto-wrap mode:
			if (s->decawm) {
				wrap(s);
			} else { // No auto-wrap, keep cursor at end:
				s->cursor.x = jbxvt.scr.chars.width - 1;
				cursor(CURSOR_DRAW);
				return;
			}
		}
#endif
	}
#if 0
	if (s->cursor.x >= jbxvt.scr.chars.width) {
		s->cursor.x = jbxvt.scr.chars.width - 1;
		s->wrap_next = s->decawm;
	}
#endif
	cursor(CURSOR_DRAW);
}


