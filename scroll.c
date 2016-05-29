/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scroll.h"

#include "config.h"
#include "jbxvt.h"
#include "log.h"
#include "repair_damage.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "show_selection.h"

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

// Save lines that scroll off the top of the screen.
static void free_top_lines(const int16_t count)
{
	LOG("free_top_lines()");
	for (uint16_t i = 1; i <= count; ++i) {
		struct slinest ** s = &jbxvt.scr.sline.data
			[jbxvt.scr.sline.max - i];
		if(*s) {
			free((*s)->sl_text);
			free((*s)->sl_rend);
			(*s)->sl_length = 0;
			free(*s);
		}
	}
}

static void set_selend_index(const int i, const int count,
	struct selst * restrict s)
{
	if (s->se_type == SAVEDSEL && s->se_index == i)
		  s->se_index = i + count;
}

static void transmogrify(const int16_t j, const int8_t count)
{
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

static int16_t sc_up_find_col(uint8_t * restrict s)
{
	int16_t col;
	for (col = jbxvt.scr.chars.width - 1;
		col >= 0 && s[col] == 0; --col)
		  ;
	return col + 1;
}

static void lclr(const uint8_t i, const uint8_t j, const size_t sz,
	void ** t, void ** s)
{
	memset(s[i], 0, sz);
	t[j] = s[i];
}

static void clr_ln_s_r(uint8_t i, uint8_t j, uint8_t ** save,
	uint32_t ** rend)
{
	const size_t sz = jbxvt.scr.chars.width + 1;
	lclr(i, j, sz, (void**)jbxvt.scr.current->text, (void**)save);
	lclr(i, j, sz * sizeof(uint32_t),
		(void**)jbxvt.scr.current->rend, (void**)rend);
}

static void clr_lines(int8_t count, const uint8_t rc,
	uint8_t ** save, uint32_t ** rend, const bool up)
{
	while(count--)
		clr_ln_s_r(count, up ? rc - count - 1 : rc + count,
			save, rend);
}

static void sc_up_cp_rows(const int8_t count)
{
	Point iter;
	for (iter.row = 0; iter.row < count; ++iter.row) {
		uint8_t * s = jbxvt.scr.current->text[iter.row];
		uint32_t * r = jbxvt.scr.current->rend[iter.row];
		iter.col = sc_up_find_col(s);
		struct slinest *sl = malloc(sizeof(struct slinest));
		sl->sl_text = malloc(iter.col + 1);
		memcpy(sl->sl_text, s, iter.col);
		sl->sl_text[iter.col] = s[jbxvt.scr.chars.width];
		if (!r[jbxvt.scr.chars.width])
			  sl->sl_rend = NULL;
		else {
			sl->sl_rend = malloc((iter.col + 1)
				* sizeof(uint32_t));
			memcpy(sl->sl_rend, r, iter.col
				* sizeof(uint32_t));
		}
		sl->sl_length = iter.col;
		jbxvt.scr.sline.data[count - iter.row - 1] = sl;
		sel_scr_to_sav(&jbxvt.sel.end1, iter.row, count);
		sel_scr_to_sav(&jbxvt.sel.end2, iter.row, count);
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

	XCopyArea(jbxvt.X.dpy, jbxvt.X.win.vt, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, 0, y[0], jbxvt.scr.pixels.width,
		height, 0, y[1]);
	repair_damage();
}

static void sc_up(const uint8_t row1, uint8_t row2,
	const int8_t count)
{
	LOG("scroll_up(count: %d, row1: %d, row2: %d)", count, row1, row2);
	if (row1 == 0 && jbxvt.scr.current == &jbxvt.scr.s1) {
		free_top_lines(count);
		Point iter;
		for (iter.row = jbxvt.scr.sline.max - count - 1;
			iter.row >= 0; --iter.row) {
			jbxvt.scr.sline.data[iter.row + count]
				= jbxvt.scr.sline.data[iter.row];
			set_selend_index(iter.row, count, &jbxvt.sel.end1);
			set_selend_index(iter.row, count, &jbxvt.sel.end2);
		}
		sc_up_cp_rows(count);
		jbxvt.scr.sline.top = constrain(jbxvt.scr.sline.top
			+ count, jbxvt.scr.sline.max + 1);
		sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
			jbxvt.scr.offset, jbxvt.scr.offset
			+ jbxvt.scr.chars.height - 1);
	}
	uint8_t *save[MAX_SCROLL];
	uint32_t *rend[MAX_SCROLL];
	uint8_t j = row1;
	for (uint8_t i = 0; i < count; ++i, ++j) {
		save[i] = jbxvt.scr.current->text[j];
		rend[i] = jbxvt.scr.current->rend[j];
		ck_sel_on_scr(j);
	}

	for(row2++; j < row2; ++j)
		transmogrify(j, -count);
	clr_lines(count, row2, save, rend, true);
	if (count < row2 - row1)
		cp_repair(row1, row2, count, true);
	const int16_t y = MARGIN + (row2 - count) * jbxvt.X.font_height;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
		0, y, jbxvt.scr.pixels.width,
		count * jbxvt.X.font_height,False);
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
	clr_lines(count, row1, save, rend, false);
	if (count < row2 - row1)
		  cp_repair(row1, row2, count, false);
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt, 0,
		MARGIN + row1 * jbxvt.X.font_height,
		jbxvt.scr.pixels.width, count * jbxvt.X.font_height,
		false);
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
	if(count < 0)
		sc_dn(row1, row2, count);
	else
			sc_up(row1, row2, count);
}

