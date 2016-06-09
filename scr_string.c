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


static void handle_new_lines(int8_t nlcount)
{
	struct screenst * restrict c = jbxvt.scr.current;
	nlcount -= c->margin.bottom - c->cursor.y;
	const uint8_t lim = c->cursor.y - c->margin.top - 1;
	nlcount = nlcount < 0 ? 0 : nlcount > lim ? lim : nlcount;
	if (nlcount > MAX_SCROLL)
		  nlcount = MAX_SCROLL;
	scroll(c->margin.top, c->margin.bottom, nlcount);
	c->cursor.y -= nlcount;
}

#if defined(__i386__) || defined(__amd64__)
       __attribute__((regparm(1)))
#endif//x86
static void handle_insert(uint8_t n, const xcb_point_t p)
{
	LOG("handle_insert(n=%d, p={%d, %d})", n, p.x, p.y);
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
	struct screenst * restrict c = jbxvt.scr.current;
	c->text [c->cursor.y]
		[jbxvt.scr.chars.width] = 1;
	if (c->cursor.y == c->margin.bottom)
		  scroll(c->margin.top,
			  c->margin.bottom, 1);
	else if (c->cursor.y < jbxvt.scr.chars.height - 1)
		  ++c->cursor.y;
	c->cursor.x = c->wrap_next = 0;
}

/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void scr_string(uint8_t * restrict str, int8_t len, int8_t nlcount)
{
#ifdef SCR_DEBUG
	LOG("scr_string(s, len: %d, nlcount: %d)\n", len, nlcount);
#endif//SCR_DEBUG
	xcb_point_t p;
	home_screen();
	cursor(CURSOR_DRAW);
	if (nlcount > 0)
		  handle_new_lines(nlcount);
	struct screenst * restrict c = jbxvt.scr.current;
	while (len) {
		if (likely(*str == '\r')) { // carriage return
			c->cursor.x = 0;
			c->wrap_next = 0;
			--len;
			++str;
			continue;
		} else if (*str == '\n') { // line feed
			if (likely(c->cursor.y
				< jbxvt.scr.chars.height - 1)) {
				++c->cursor.y;
			} else if (c->cursor.y
				== c->margin.bottom) {
				scroll(c->margin.top,
					c->margin.bottom,1);
			}
			check_selection(c->cursor.y,
				c->cursor.y);
			c->wrap_next = 0;
			--len;
			++str;
			continue;
		} else if (unlikely(*str == '\t')) {
			if (c->cursor.x < jbxvt.scr.chars.width - 1) {
				uint8_t * s = c->text[c->cursor.y];
				if (s[c->cursor.x] == 0)
					  s[c->cursor.x] = '\t';
				++c->cursor.x;
				// Advance to next tab stop:
				while (c->cursor.x % 8 && c->cursor.x
					< jbxvt.scr.chars.width - 1)
					  ++c->cursor.x;
			}
			--len;
			++str;
			continue;
		}

		if (c->wrap_next)
			  handle_wrap_next();

		check_selection(c->cursor.y, c->cursor.y);
		p.x = MARGIN + jbxvt.X.font_width * c->cursor.x;
		p.y = MARGIN + jbxvt.X.font_height * c->cursor.y;
		uint_fast16_t n = 0;
		while (str[++n] >= ' ')
			  ;
		if (unlikely(n + c->cursor.x > jbxvt.scr.chars.width))
			  n = jbxvt.scr.chars.width - c->cursor.x;
		if (unlikely(c->insert))
			  handle_insert(n, p);
		memcpy(c->text[c->cursor.y] + c->cursor.x, str, n);
		paint_rval_text(str,jbxvt.scr.rstyle, n, p);
		for (int_fast8_t i = n - 1; jbxvt.scr.rstyle && i >= 0; --i)
			c->rend[c->cursor.y][c->cursor.x + i]
				= jbxvt.scr.rstyle;
		len -= n;
		str += n;
		c->cursor.x += n;
		if (unlikely(len > 0 && c->cursor.x == jbxvt.scr.chars.width
			&& *str >= ' ')) {
			c->text [c->cursor.y] [jbxvt.scr.chars.width] = 1;
			if (likely(c->cursor.y == c->margin.bottom)) {
				scroll(c->margin.top, c ->margin.bottom, 1);
			} else {
				++c->cursor.y;
			}
			c->cursor.x = 0;
		}
	}
	if (c->cursor.x >= jbxvt.scr.chars.width) {
		c->cursor.x = jbxvt.scr.chars.width - 1;
		c->wrap_next = c->wrap;
	}
	cursor(CURSOR_DRAW);
}


