/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "scroll.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"

#include <gc.h>
#include <stdlib.h>
#include <string.h>

static void sel_scr_to_sav(SelEnd * restrict s,
	const int i, const int count)
{
	if (s->type == SCREENSEL && s->index == i) {
		s->type = SAVEDSEL;
		s->index = count - i - 1;
	}
}

static void transmogrify(const int16_t j, const int8_t count,
	VTScreen * restrict s)
{
	const int16_t k = j + count;
	s->text[k] = s->text[j];
	s->rend[k] = s->rend[j];
	if (jbxvt.sel.end[0].type == SCREENSEL
		&& jbxvt.sel.end[0].index == j)
		  jbxvt.sel.end[0].index = k;
	if (jbxvt.sel.end[1].type == SCREENSEL
		&& jbxvt.sel.end[1].index == j)
		  jbxvt.sel.end[1].index = k;
}
static void ck_sel_on_scr(const int j)
{
	// clear selection if it scrolls off screen:
	if (jbxvt.sel.end[0].index == j
		|| jbxvt.sel.end[1].index == j)
		  scr_clear_selection();
}

static uint16_t find_col(uint8_t * restrict s, uint_fast16_t c)
{
	return s[c] ? find_col(s, c + 1) : c;
}

static void clear(int8_t count, const uint8_t rc,
	uint8_t ** text, uint32_t ** rend, const bool up)
{
	if(--count < 0)
		  return;
	const uint16_t sz = jbxvt.scr.chars.width;
	memset(text[count], 0, sz + 1);
	memset(rend[count], 0, sz << 2);
	VTScreen * cur = jbxvt.scr.current;
	const uint8_t j = rc + (up ? - count - 1 : count);
	cur->text[j] = text[count];
	cur->rend[j] = rend[count];
	clear(count, rc, text, rend, up);
}

static SLine * new_sline(const uint16_t x)
{
	SLine * sl = GC_MALLOC(sizeof(SLine));
	sl->sl_length = x;
	sl->sl_text = GC_MALLOC(x + 1);
	sl->sl_rend = GC_MALLOC(x << 2);
	return sl;
}

static void cp_rows(int16_t i, const int16_t count)
{
#ifdef SCROLL_DEBUG
	LOG("cp_rows(i: %d, count: %d)", i, count);
#endif//SCROLL_DEBUG
	if (--i < 0)
		  return;
	VTScreen * s = jbxvt.scr.current;
	uint8_t * t = s->text[i];
	const uint16_t x = find_col(t, 0);
	uint32_t * r = s->rend[i];
	SLine * sl = new_sline(x);
	memcpy(sl->sl_text, t, x);
	sl->wrap = t[jbxvt.scr.chars.width];
	memcpy(sl->sl_rend, r, x << 2);
	jbxvt.scr.sline.data[count - i - 1] = sl;
	sel_scr_to_sav(&jbxvt.sel.end[0], i, count);
	sel_scr_to_sav(&jbxvt.sel.end[1], i, count);
	cp_rows(i, count);
}

static void get_y(int16_t * restrict y, const uint8_t row1,
	const int8_t count, const bool up)
{
	const uint8_t fh = jbxvt.X.f.size.h;
	const int16_t a = MARGIN + row1 * fh;
	const int16_t b = a + count * fh;
	*(up ? y : y + 1) = b;
	*(up ? y + 1 : y) = a;
}

static void copy_visible(const uint8_t row1, const uint8_t row2,
	const int8_t count, const bool up)
{
	int16_t y[2];
	get_y(y, row1, count, up);
	const uint16_t height = (row2 - row1 - count)
		* jbxvt.X.f.size.h;
	xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt,
		jbxvt.X.win.vt, jbxvt.X.gc.tx, 0, y[0],
		0, y[1], jbxvt.scr.pixels.width, height);
	// the above blocks the event queue, flush it
	xcb_flush(jbxvt.X.xcb);
}

// Handle lines that scroll off the top of the screen.
static void scroll_off(const int16_t count)
{
	// -1 to avoid going over array bounds
	memcpy(jbxvt.scr.sline.data + count,
		jbxvt.scr.sline.data, count - 1);
}

static void add_scroll_history(const int8_t count)
{
	scroll_off(count);
	const uint16_t max = jbxvt.scr.sline.max;
	// -1 to avoid going over array bounds
	int_fast16_t y = max - count - 1;
	SLine ** i = &jbxvt.scr.sline.data[y],
		** j = &jbxvt.scr.sline.data[y + count];
	for (; y >= 0; --y, --i, --j)
		  *j = *i;
	cp_rows(count, count);
	uint16_t t = jbxvt.scr.sline.top + count;
	t = jbxvt.scr.sline.top = MIN(t, max);
	const uint8_t h = jbxvt.scr.chars.height - 1;
	const int16_t o = jbxvt.scr.offset;
	sbar_show(h + t, o, o + h);
}

static int8_t copy_screen_area(const int8_t i,
	const int8_t j, const int8_t mod, const int8_t count,
	uint8_t ** save, uint32_t ** rend)
{
	if(i >= count)
		  return j;
	VTScreen * restrict scr = jbxvt.scr.current;
	save[i] = scr->text[j];
	rend[i] = scr->rend[j];
	ck_sel_on_scr(j);
	return copy_screen_area(i + 1, j + mod, mod,
		count, save, rend);
}

static void clear_area(const int16_t y, const int8_t count)
{
	const uint8_t fh = jbxvt.X.f.size.h;
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, 0, MARGIN + y * fh,
		jbxvt.scr.pixels.width, count * fh);
}

void scroll1(int16_t n)
{
	cp_rows(n, n);
	jbxvt.scr.sline.top = MIN(jbxvt.scr.sline.top + n,
		jbxvt.scr.sline.max);
	for (int_fast16_t j = n;
		j < jbxvt.scr.chars.height; ++j)
		  transmogrify(j, -n, &jbxvt.scr.s[0]);
}

static void sc_dn(const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	for(int8_t j = copy_screen_area(0, row2, -1,
		count, save, rend); j >= row1; --j)
		  transmogrify(j, count, jbxvt.scr.current);
	clear(count, row1, save, rend, false);
	copy_visible(row1, row2, count, false);
	clear_area(row1, count);
}

static void sc_up(const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	if (jbxvt.scr.current == &jbxvt.scr.s[0] && row1 == 0)
		add_scroll_history(count);
	for(int8_t j = copy_screen_area(0, row1,
		1, count, save, rend); j < row2; ++j)
		transmogrify(j, -count, jbxvt.scr.current);
	clear(count, row2, save, rend, true);
	copy_visible(row1, row2, count, true);
	clear_area(row2 - count, count);
}

/*  Scroll count lines from row1 to row2 inclusive.
    row1 should be <= row2.  Scrolling is up for
    a positive count and down for a negative count.
    count is limited to a maximum of MAX_SCROLL lines.  */
void scroll(const uint8_t row1, const uint8_t row2,
	const int16_t count)
{
	LOG("scroll(%d, %d, %d)", row1, row2, count);
	// Sanitize input:
	if(!count || row1 > row2
		|| row2 >= jbxvt.scr.chars.height
		|| abs(count) > MAX_SCROLL)
		  return;
	const bool up = count > 0;
	const int16_t n = up ? count : -count;
	uint8_t *save[n];
	uint32_t *rend[n];
	// row2 + 1 to include last line
	(up ? sc_up : sc_dn)(row1, row2 + 1, n, save, rend);
	change_offset(0);
	cursor(CURSOR_DRAW); // clear
	cursor(CURSOR_DRAW);
}

