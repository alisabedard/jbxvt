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

// Free lines that scroll off the top of the screen.
static void free_top(int16_t count)
{
	if (--count < 0)
		  return;
	int16_t i = jbxvt.scr.sline.max - count;
	struct slinest * s = jbxvt.scr.sline.data[i];
	if (s) {
		// Guard against double-free:
		if(s->canary != 0)
			  return;
		else
			  s->canary=0xff;
#if 0
		if(s->sl_text) {
			free(s->sl_text);
			if(s->sl_rend)
				  free(s->sl_rend);
		}
		free(s);
#endif
	}
	--i;
	jbxvt.scr.sline.data[i + count] = jbxvt.scr.sline.data[i];
	//free_top(count - 1);
	free_top(count);
}

static void transmogrify(const int16_t j, const int8_t count)
{
	//LOG("transmogrify(j: %d, count: %d)", j, count);
	const int16_t k = j + count;
	jbxvt.scr.current->text[k]
		= jbxvt.scr.current->text[j];
	jbxvt.scr.current->rend[k]
		= jbxvt.scr.current->rend[j];
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

static void sc_up_cp_rows(const int8_t count)
{
	LOG("sc_up_cp_rows(count = %d)", count);
	xcb_point_t iter;
	for (iter.y = count - 1; iter.y >= 0; --iter.y) {
		uint8_t * s = jbxvt.scr.current->text[iter.y];
		if(!s)
			  continue;
		uint32_t * r = jbxvt.scr.current->rend[iter.y];
		iter.x = find_col(s, 0);
		//struct slinest *sl = calloc(1, sizeof(struct slinest));
		struct slinest *sl = GC_MALLOC(sizeof(struct slinest));
		// +1 to have last byte as wrap flag:
		sl->sl_text = GC_MALLOC(iter.x + 1);
		memcpy(sl->sl_text, s, iter.x);
		// copy wrap flag:
		sl->sl_text[iter.x] = s[jbxvt.scr.chars.width];
		/* iter.x, not iter.x + 1, since the last byte
		   of the sl_text, only, is used for the wrap flag.  */
		const uint16_t rendsz = iter.x * sizeof(uint32_t);
		sl->sl_rend = GC_MALLOC(rendsz);
		memcpy(sl->sl_rend, r, rendsz);
		sl->sl_length = iter.x;
		jbxvt.scr.sline.data[count - iter.y - 1] = sl;
		sel_scr_to_sav(&jbxvt.sel.end1, iter.y, count);
		sel_scr_to_sav(&jbxvt.sel.end2, iter.y, count);
	}
}

static void cp_repair(const uint8_t row1, const uint8_t row2,
	const int8_t count, const bool up)
{
	int16_t a, b, y[2];
	a = MARGIN + row1 * jbxvt.X.font_height;
	b = a + count * jbxvt.X.font_height;
	const uint16_t height = (row2 - row1 - count)
		* jbxvt.X.font_height;
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

static void add_scroll_history(const int8_t count)
{
	free_top(count);
	int_fast16_t y;
	for (y = jbxvt.scr.sline.max - count - 1;
		y >= 0; --y) {
		jbxvt.scr.sline.data[y + count]
			= jbxvt.scr.sline.data[y];
	}
	sc_up_cp_rows(count);
	const uint16_t t = jbxvt.scr.sline.top + count;
	const uint16_t max = jbxvt.scr.sline.max;
	jbxvt.scr.sline.top = likely(t < max) ? t : max;
	sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
		jbxvt.scr.offset, jbxvt.scr.offset
		+ jbxvt.scr.chars.height - 1);

}

void scroll1(int16_t count)
{
	LOG("scroll1(%d)", count);
	while (count > 0) {
		// If count > MAX_SCROLL, scroll in installments
		int_fast16_t n = count > MAX_SCROLL ? MAX_SCROLL : count;
		count -= n;
		// free lines that scroll off the top
		free_top(n);
		sc_up_cp_rows(n);
		jbxvt.scr.sline.top += n;
		if (jbxvt.scr.sline.top > jbxvt.scr.sline.max)
			  jbxvt.scr.sline.top = jbxvt.scr.sline.max;
		for (int_fast16_t j = n; j < jbxvt.scr.chars.height; ++j) {
			jbxvt.scr.s1.text[j - n] = jbxvt.scr.s1.text[j];
			jbxvt.scr.s1.rend[j - n] = jbxvt.scr.s1.rend[j];
		}
	}
}


static void sc_up(const uint8_t row1, uint8_t row2, int8_t count)
{
	LOG("scroll_up(count: %d, row1: %d, row2: %d)", count, row1, row2);
	struct screenst * scr = jbxvt.scr.current;
	if (scr == &jbxvt.scr.s1 && row1 == 0)
		add_scroll_history(count);
	uint8_t *save[MAX_SCROLL];
	uint32_t *rend[MAX_SCROLL];
	uint_fast8_t j = row1;
	for (uint_fast8_t i = 0; i < count; ++i, ++j) {
		save[i] = scr->text[j];
		rend[i] = scr->rend[j];
		ck_sel_on_scr(j);
	}
	for(++row2; j < row2; ++j)
		transmogrify(j, -count);
	clear(count, row2, save, rend, true);
	cp_repair(row1, row2, count, true);
	const int16_t y = MARGIN + (row2 - count) * jbxvt.X.font_height;
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, 0, y,
		jbxvt.scr.pixels.width, count * jbxvt.X.font_height);
}

static void sc_dn(uint8_t row1, uint8_t row2, int8_t count)
{
	LOG("scroll_down(%d, %d, %d)", row1, row2, count);
	count = -count;
	uint32_t *rend[MAX_SCROLL];
	uint8_t *save[MAX_SCROLL];
	int16_t j = row2 - 1;
	for (int16_t i = 0; i < count; ++i, --j) {
		save[i] = jbxvt.scr.current->text[j];
		rend[i] = jbxvt.scr.current->rend[j];
		ck_sel_on_scr(j);
	}
	while(j >= row1)
		  transmogrify(j--, count);
	clear(count, row1, save, rend, false);
	cp_repair(row1, row2, count, false);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, 0,
		MARGIN + row1 * jbxvt.X.font_height,
		jbxvt.scr.pixels.width, count * jbxvt.X.font_height);
}

/*  Scroll count lines from row1 to row2 inclusive.  row1 should be <= row2.
 *  scrolling is up for a +ve count and down for a -ve count.
 *  count is limited to a maximum of MAX_SCROLL lines.
 */
void scroll(const uint8_t row1, const uint8_t row2, const int16_t count)
{
	LOG("scroll(%d, %d, %d)", row1, row2, count);
	// Sanitize input:
	if(row1 > row2)
		  return;
	if(abs(count) > MAX_SCROLL)
		  return;
	if (count >= 0)
		sc_up(row1, row2, count);
	else
		sc_dn(row1, row2, count);
	home_screen();
	cursor(CURSOR_DRAW);
	cursor(CURSOR_DRAW);
}

