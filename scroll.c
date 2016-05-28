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
static void free_top_lines(int count)
{
	LOG("free_top_lines()");
	for (int i = 1; i <= count; i++) {
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

static void transmogrify(const int j, const int8_t count)
{
	jbxvt.scr.current->text[j + count]
		= jbxvt.scr.current->text[j];
	jbxvt.scr.current->rend[j + count]
		= jbxvt.scr.current->rend[j];
	if (jbxvt.sel.end1.se_type == SCREENSEL
		&& jbxvt.sel.end1.se_index == j)
		  jbxvt.sel.end1.se_index = j + count;
	if (jbxvt.sel.end2.se_type == SCREENSEL
		&& jbxvt.sel.end2.se_index == j)
		  jbxvt.sel.end2.se_index = j + count;
}

static void ck_sel_on_scr(const int j)
{
	// clear selection if it scrolls off screen:
	if (jbxvt.sel.end1.se_index == j
		|| jbxvt.sel.end2.se_index == j)
		  scr_clear_selection();
}

static void sc_up(uint8_t row1, uint8_t row2, int8_t count)
{
	LOG("scroll_up(count: %d, row1: %d, row2: %d)", count, row1, row2);
	if (row1 == 0 && jbxvt.scr.current == &jbxvt.scr.s1) {
		free_top_lines(count);
		for (int i = jbxvt.scr.sline.max - count - 1;
			i >= 0; i--) {
			jbxvt.scr.sline.data[i + count]
				= jbxvt.scr.sline.data[i];
			set_selend_index(i, count, &jbxvt.sel.end1);
			set_selend_index(i, count, &jbxvt.sel.end2);
		}
		for (uint8_t i = 0; i < count; i++) {
			uint8_t * s = jbxvt.scr.current->text[i];
			uint32_t * r = jbxvt.scr.current->rend[i];
			int j;
			for (j = jbxvt.scr.chars.width - 1;
				j >= 0 && s[j] == 0; j--)
				  ;
			j++;
			struct slinest *sl = malloc(sizeof(struct slinest));
			sl->sl_text = malloc(j+1);
			memcpy(sl->sl_text,s,j);
			sl->sl_text[j] = s[jbxvt.scr.chars.width];
			if (!r[jbxvt.scr.chars.width])
				  sl->sl_rend = NULL;
			else {
				sl->sl_rend = malloc((j + 1)*sizeof(uint32_t));
				memcpy(sl->sl_rend,r,j*sizeof(uint32_t));
			}
			sl->sl_length = j;
			jbxvt.scr.sline.data[count - i - 1] = sl;
			sel_scr_to_sav(&jbxvt.sel.end1, i, count);
			sel_scr_to_sav(&jbxvt.sel.end2, i, count);
		}
		jbxvt.scr.sline.top += count;
		if (jbxvt.scr.sline.top > jbxvt.scr.sline.max)
			  jbxvt.scr.sline.top = jbxvt.scr.sline.max;
		sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
			jbxvt.scr.offset, jbxvt.scr.offset
			+ jbxvt.scr.chars.height - 1);
	}
	int j = row1;
	uint8_t *save[MAX_SCROLL];
	uint32_t *rend[MAX_SCROLL];
	for (int i = 0; i < count; i++, j++) {
		save[i] = jbxvt.scr.current->text[j];
		rend[i] = jbxvt.scr.current->rend[j];
		ck_sel_on_scr(j);
	}

	for(row2++; j < row2; j++)
		transmogrify(j, -count);

	for (int i = 0; i < count; i++) {
		memset(save[i],0,jbxvt.scr.chars.width + 1);
		jbxvt.scr.current->text[row2 - i - 1] = save[i];
		memset(rend[i],0,(jbxvt.scr.chars.width + 1)*sizeof(uint32_t));
		jbxvt.scr.current->rend[row2 - i - 1] = rend[i];
	}
	if (count < row2 - row1) {
		const int16_t y2 = MARGIN + row1
			* jbxvt.X.font_height;
		const int16_t y1 = y2 + count * jbxvt.X.font_height;
		int height = (row2 - row1 - count)
			* jbxvt.X.font_height;
		XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,
			jbxvt.X.win.vt,jbxvt.X.gc.tx,
			0,y1,jbxvt.scr.pixels.width,height,0,y2);
		repair_damage();
	}
	const int16_t y = MARGIN + (row2 - count) * jbxvt.X.font_height;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
		0, y, jbxvt.scr.pixels.width,
		count * jbxvt.X.font_height,False);
}

static void lclr(const int16_t i, const uint8_t row1, const size_t sz,
	uint32_t ** t, uint32_t ** s)
{
	memset(s[i], 0, sz);
	t[row1 + i] = s[i];
}

static void clear_lines(int16_t i, uint8_t row1,
	uint8_t ** ts, uint32_t ** tr,
	uint8_t ** ss, uint32_t ** sr)
{
	const size_t s = jbxvt.scr.chars.width + 1;
	lclr(i, row1, s, (uint32_t **)ts, (uint32_t **)ss);
	lclr(i, row1, s * sizeof(uint32_t), tr, sr);
}

static void copy_and_repair(const uint8_t row1, const uint8_t row2, int8_t count)
{
	int16_t y1 = MARGIN + row1 * jbxvt.X.font_height,
		y2 = y1 + count * jbxvt.X.font_height;
	int height = (row2 - row1 - count)
		* jbxvt.X.font_height;
	XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,
		jbxvt.X.win.vt,jbxvt.X.gc.tx,
		0,y1,jbxvt.scr.pixels.width,height,0,y2);
	repair_damage();

}

static void sc_dn(uint8_t row1, uint8_t row2, int8_t count)
{
	LOG("scroll_down(%d, %d, %d)", row1, row2, count);
	count = -count;
	uint32_t *rend[MAX_SCROLL];
	uint8_t *save[MAX_SCROLL];
	int16_t i, j = row2 - 1;
	for (i = 0; i < count; i++, j--) {
		save[i] = jbxvt.scr.current->text[j];
		rend[i] = jbxvt.scr.current->rend[j];
		ck_sel_on_scr(j);
	}
	for (; j >= row1; j--)
		transmogrify(j, count);
	for (i = 0; i < count; i++)
		clear_lines(i, row1, jbxvt.scr.current->text,
			jbxvt.scr.current->rend, save, rend);
	if (count < row2 - row1)
		  copy_and_repair(row1, row2, count);
	i = count * jbxvt.X.font_height;
	const int16_t y1 = MARGIN + row1 * jbxvt.X.font_height;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt, 0, y1,
		jbxvt.scr.pixels.width, i, false);
}

/*  Scroll count lines from row1 to row2 inclusive.  row1 should be <= row2.
 *  scrolling is up for a +ve count and down for a -ve count.
 *  count is limited to a maximum of MAX_SCROLL lines.
 */
void scroll(const uint8_t row1, const uint8_t row2, const int16_t count)
{
	LOG("scroll(%d, %d, %d)", row1, row2, count);
	// Sanitize input:
	if((row1 <= row2) && (count < MAX_SCROLL)) {
		// Negative is down:
		if(count < 0)
			sc_dn(row1, row2, count);
		else
			sc_up(row1, row2, count);
	}
}

