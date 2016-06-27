/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scroll.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "show_selection.h"

#include <gc.h>
#include <stdlib.h>
#include <string.h>

static void sel_scr_to_sav(struct selst * restrict s,
	const int i, const int count)
{
	if (s->se_type == SCREENSEL && s->se_index == i) {
		s->se_type = SAVEDSEL;
		s->se_index = count - i - 1;
	}
}

static void transmogrify(const int16_t j, const int8_t count,
	VTScreen * restrict s)
{
	const int16_t k = j + count;
	s->text[k] = s->text[j];
	s->rend[k] = s->rend[j];
	if (jbxvt.sel.end1.se_type == SCREENSEL
		&& jbxvt.sel.end1.se_index == j)
		  jbxvt.sel.end1.se_index = k;
	if (jbxvt.sel.end2.se_type == SCREENSEL
		&& jbxvt.sel.end2.se_index == j)
		  jbxvt.sel.end2.se_index = k;
}
static void ck_sel_on_scr(const int j)
{
	// clear selection if it scrolls off screen:
	if (jbxvt.sel.end1.se_index == j
		|| jbxvt.sel.end2.se_index == j)
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
	memset(rend[count], 0, sz<<2);
	struct screenst * cur = jbxvt.scr.current;
	uint8_t j = up ? rc - count - 1: rc + count;
	cur->text[j] = text[count];
	cur->rend[j] = rend[count];
	clear(count, rc, text, rend, up);
}

static void cp_rows(int16_t i, const int16_t count)
{
	LOG("cp_rows(i: %d, count: %d)", i, count);
	if (--i < 0)
		  return;
	VTScreen * s = jbxvt.scr.current;
	uint8_t * t = s->text[i];
	const uint16_t x = find_col(t, 0);
	uint32_t * r = s->rend[i];
	SLine * sl = GC_MALLOC(sizeof(SLine));
	sl->sl_length = x;
	sl->sl_text = GC_MALLOC(x + 1);
	sl->sl_rend = GC_MALLOC(x << 2);
	memcpy(sl->sl_text, t, x);
	// copy wrap flag:
	sl->sl_text[x] = t[jbxvt.scr.chars.width];
	memcpy(sl->sl_rend, r, x << 2);
	jbxvt.scr.sline.data[count - i - 1] = sl;
	sel_scr_to_sav(&jbxvt.sel.end1, i, count);
	sel_scr_to_sav(&jbxvt.sel.end2, i, count);
	cp_rows(i, count);
}

static void cp_repair(const uint8_t row1, const uint8_t row2,
	const int8_t count, const bool up)
{
	int16_t a, b, y[2];
	a = MARGIN + row1 * jbxvt.X.font_size.h;
	b = a + count * jbxvt.X.font_size.h;
	const uint16_t height = (row2 - row1 - count)
		* jbxvt.X.font_size.h;
	if(up) {
		y[0] = b;
		y[1] = a;
	} else {
		y[0] = a;
		y[1] = b;
	}
	xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, 0, y[0], 0, y[1], jbxvt.scr.pixels.width,
		height);
	// the above blocks the event queue, flush it
	xcb_flush(jbxvt.X.xcb);
}

// Handle lines that scroll off the top of the screen.
static void scroll_off(int_fast16_t count)
{
	if (--count < 0)
		  return;
	const int16_t i = jbxvt.scr.sline.max - count - 1;
	jbxvt.scr.sline.data[i + count] = jbxvt.scr.sline.data[i];
	scroll_off(count);
}

static void add_scroll_history(const int8_t count)
{
	scroll_off(count);
	const uint16_t max = jbxvt.scr.sline.max;
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
	return copy_screen_area(i + 1, j + mod, mod, count, save, rend);
}

static void sc_up(const uint8_t row1, uint8_t row2, int8_t count)
{
	LOG("scroll_up(count: %d, row1: %d, row2: %d)", count, row1, row2);
	if (jbxvt.scr.current == &jbxvt.scr.s1 && row1 == 0)
		add_scroll_history(count);
	uint8_t *save[MAX_SCROLL] = {NULL};
	uint32_t *rend[MAX_SCROLL] = {NULL};
	++row2;
	for(int8_t j = copy_screen_area(0, row1, 1, count, save, rend);
		j < row2; ++j)
		transmogrify(j, -count, jbxvt.scr.current);
	clear(count, row2, save, rend, true);
	cp_repair(row1, row2, count, true);
	const int16_t y = MARGIN + (row2 - count) * jbxvt.X.font_size.h;
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, 0, y,
		jbxvt.scr.pixels.width, count * jbxvt.X.font_size.h);
}

void scroll1(int16_t count)
{
	LOG("scroll1(%d)", count);
	if(count <= 0)
		  return;
	// If count > MAX_SCROLL, scroll in installments
	const int16_t n = MIN(count, MAX_SCROLL);
	count -= n;
	cp_rows(n, n);
	jbxvt.scr.sline.top = MIN(jbxvt.scr.sline.top + n,
		jbxvt.scr.sline.max);
	for (int_fast16_t j = n; j < jbxvt.scr.chars.height; ++j)
		  transmogrify(j, -n, &jbxvt.scr.s1);
	scroll1(count);
}

static void sc_dn(uint8_t row1, const uint8_t row2, int8_t count)
{
	LOG("scroll_down(%d, %d, %d)", row1, row2, count);
	count = -count;
	uint32_t *rend[MAX_SCROLL];
	uint8_t *save[MAX_SCROLL];
	for(int8_t j = copy_screen_area(0, row2 - 1, -1, count, save, rend);
		j >= row1; --j)
		  transmogrify(j, count, jbxvt.scr.current);
	clear(count, row1, save, rend, false);
	cp_repair(row1, row2 + 1, count, false);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, 0,
		MARGIN + row1 * jbxvt.X.font_size.h,
		jbxvt.scr.pixels.width, count * jbxvt.X.font_size.h);
}

/*  Scroll count lines from row1 to row2 inclusive.  row1 should be <= row2.
 *  scrolling is up for a +ve count and down for a -ve count.
 *  count is limited to a maximum of MAX_SCROLL lines.
 */
void scroll(const uint8_t row1, const uint8_t row2, const int16_t count)
{
	LOG("scroll(%d, %d, %d)", row1, row2, count);
	// Sanitize input:
	if(!count || row1 > row2 || row2 >= jbxvt.scr.chars.height
		|| abs(count) > MAX_SCROLL)
		  return;
	(count > 0 ? &sc_up : &sc_dn)(row1, row2, count);
	home_screen();
	cursor(CURSOR_DRAW);
	cursor(CURSOR_DRAW);
}

