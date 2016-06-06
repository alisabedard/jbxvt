/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_string.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "repair_damage.h"
#include "screen.h"
#include "scroll.h"
#include "selection.h"

#include <string.h>

static uint8_t handle_new_lines(int8_t nlcount)
{
	if (nlcount > 0) {
		if (jbxvt.scr.current->cursor.y
			> jbxvt.scr.current->margin.bottom)
			nlcount = 0;
		else
			nlcount -= jbxvt.scr.current->margin.bottom
				- jbxvt.scr.current->cursor.y;
		nlcount = constrain(nlcount, jbxvt.scr.current->cursor.y
			- jbxvt.scr.current->margin.top + 1);
		if (nlcount > MAX_SCROLL)
			nlcount = MAX_SCROLL;
		scroll(jbxvt.scr.current->margin.top,
			jbxvt.scr.current->margin.bottom,nlcount);
		jbxvt.scr.current->cursor.y -= nlcount;
	}
	return nlcount;
}

#if defined(__i386__) || defined(__amd64__)
       __attribute__((regparm(1)))
#endif//x86
static void handle_insert(uint8_t n, const xcb_point_t p)
{
	LOG("handle_insert(n=%d, p={%d, %d})", n, p.x, p.y);
	uint8_t * s = jbxvt.scr.current->text
		[jbxvt.scr.current->cursor.y];
	uint32_t * r = jbxvt.scr.current->rend
		[jbxvt.scr.current->cursor.y];
	for (int_fast16_t i = jbxvt.scr.chars.width;
		i >= jbxvt.scr.current->cursor.x + n; --i) {
		s[i] = s[i - n];
		r[i] = r[i - n];
	}
	const uint16_t width = (jbxvt.scr.chars.width
		- jbxvt.scr.current->cursor.x - n)
		* jbxvt.X.font_width;
	const int16_t x2 = p.x + n * jbxvt.X.font_width;
	if (width > 0) {
		xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
			jbxvt.X.gc.ne, p.x, p.y, x2, p.y, width,
			jbxvt.X.font_height);
		repair_damage();
	}
}

static void handle_wrap_next(void)
{
	jbxvt.scr.current->text [jbxvt.scr.current->cursor.y]
		[jbxvt.scr.chars.width] = 1;
	if (jbxvt.scr.current->cursor.y == jbxvt.scr.current->margin.bottom)
		  scroll(jbxvt.scr.current->margin.top,
			  jbxvt.scr.current->margin.bottom,1);
	else if (jbxvt.scr.current->cursor.y < jbxvt.scr.chars.height - 1)
		  ++jbxvt.scr.current->cursor.y;
	jbxvt.scr.current->cursor.x = 0;
	jbxvt.scr.current->wrap_next = 0;
}

/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void scr_string(uint8_t * restrict str, int8_t len, int8_t nlcount)
{
#ifdef SCR_DEBUG
	LOG("scr_string(s, len: %d, nlcount: %d)\n", len, nlcount);
#endif//SCR_DEBUG
	uint8_t *s;
	int_fast32_t n, i;
	xcb_point_t p;

	home_screen();
	cursor(CURSOR_DRAW);
	nlcount = handle_new_lines(nlcount);
	while (len) {
		switch(*str) {
		case '\n':
			if (jbxvt.scr.current->cursor.y
				== jbxvt.scr.current->margin.bottom)
				  scroll(jbxvt.scr.current->margin.top,
					  jbxvt.scr.current->margin.bottom,1);
			else if (jbxvt.scr.current->cursor.y
				< jbxvt.scr.chars.height - 1)
				  jbxvt.scr.current->cursor.y++;
			check_selection(jbxvt.scr.current->cursor.y,
				jbxvt.scr.current->cursor.y);
			jbxvt.scr.current->wrap_next = 0;
			--len;
			++str;
			continue;
		case '\r':
			jbxvt.scr.current->cursor.x = 0;
			jbxvt.scr.current->wrap_next = 0;
			--len;
			++str;
			continue;
		case '\t':
			if (jbxvt.scr.current->cursor.x
				< jbxvt.scr.chars.width - 1) {
				s = jbxvt.scr.current->text
					[jbxvt.scr.current->cursor.y];
				if (s[jbxvt.scr.current->cursor.x] == 0)
					  s[jbxvt.scr.current->cursor.x]
						  = '\t';
				jbxvt.scr.current->cursor.x++;
				while (jbxvt.scr.current->cursor.x % 8
					&& jbxvt.scr.current->cursor.x
					< jbxvt.scr.chars.width - 1)
					  jbxvt.scr.current->cursor.x++;
			}
			--len;
			++str;

			continue;
		}

		if (jbxvt.scr.current->wrap_next)
			  handle_wrap_next();

		check_selection(jbxvt.scr.current->cursor.y,
			jbxvt.scr.current->cursor.y);
		p.x = MARGIN + jbxvt.X.font_width
			* jbxvt.scr.current->cursor.x;
		p.y = MARGIN + jbxvt.X.font_height
			* jbxvt.scr.current->cursor.y;
		for (n = 0; str[n] >= ' '; ++n)
			;
		if (n + jbxvt.scr.current->cursor.x > jbxvt.scr.chars.width)
			  n = jbxvt.scr.chars.width
				  - jbxvt.scr.current->cursor.x;

		if (unlikely(jbxvt.scr.current->insert))
			  handle_insert(n, p);

		memcpy(jbxvt.scr.current->text[jbxvt.scr.current->cursor.y]
			+ jbxvt.scr.current->cursor.x,str,n);
		/* Clear memory cells which are not part of the
			desired output string.  */
		memset(jbxvt.scr.current->text[jbxvt.scr.current->cursor.y]
			+ jbxvt.scr.current->cursor.x + n, 0,
			jbxvt.scr.chars.width
			- jbxvt.scr.current->cursor.x - n);
#ifdef SCR_DEBUG
		LOG("n: %d, strlen: %lu", n, strlen((const char *)
			jbxvt.scr.current->text[jbxvt.scr.current->cursor.y]));
#endif//SCR_DEBUG

		paint_rval_text(str,jbxvt.scr.rstyle,n,p);
		if (jbxvt.scr.rstyle == 0)
			memset(jbxvt.scr.current->rend
				[jbxvt.scr.current->cursor.y]
				+ jbxvt.scr.current->cursor.x,
				0,n*sizeof(uint32_t));
		else {
			for (i = 0; i < n; i++)
				jbxvt.scr.current->rend
					[jbxvt.scr.current->cursor.y]
					[jbxvt.scr.current->cursor.x + i]
					= jbxvt.scr.rstyle;
			jbxvt.scr.current->rend[jbxvt.scr.current->cursor.y]
				[jbxvt.scr.chars.width] = 1;
		}
		len -= n;
		str += n;
		jbxvt.scr.current->cursor.x += n;
		if (len > 0 && jbxvt.scr.current->cursor.x
			== jbxvt.scr.chars.width && *str >= ' ') {
			if (jbxvt.scr.current->wrap) {
				jbxvt.scr.current->text
					[jbxvt.scr.current->cursor.y]
					[jbxvt.scr.chars.width] = 1;
				if (jbxvt.scr.current->cursor.y
					== jbxvt.scr.current->margin.bottom)
					scroll(jbxvt.scr.current->margin.top,
						jbxvt.scr.current
						->margin.bottom, 1);
				else
				++jbxvt.scr.current->cursor.y;
				jbxvt.scr.current->cursor.x = 0;
			} else {
				jbxvt.scr.current->cursor.x
					= jbxvt.scr.chars.width - 1;
				cursor(CURSOR_DRAW);
				return;
			}
		}
	}
	if (jbxvt.scr.current->cursor.x == jbxvt.scr.chars.width) {
		jbxvt.scr.current->cursor.x = jbxvt.scr.chars.width - 1;
		jbxvt.scr.current->wrap_next = jbxvt.scr.current->wrap;
	}
	cursor(CURSOR_DRAW);
}


