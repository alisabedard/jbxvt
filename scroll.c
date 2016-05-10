#include "scroll.h"


#include "jbxvt.h"
#include "repair_damage.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "show_selection.h"
#include "slinest.h"
#include "xvt.h"

#include <stdlib.h>
#include <string.h>

static void sel_scr_to_sav(struct selst * restrict s,
	const uint8_t i, const uint8_t count)
{
	if (s->se_type == SCREENSEL && s->se_index == i) {
		s->se_type = SAVEDSEL;
		s->se_index = count - i - 1;
	}
}

static void save_top_line(uint8_t i)
{
	struct slinest * sl = jbxvt.scr.sline.data[jbxvt.scr.sline.max - i];
	if(!sl) return;
	free(sl->sl_text);
	if (sl->sl_rend != NULL)
		  free(sl->sl_rend);
	free((void *)sl);
}

static void save_top_lines_show_selection(struct selst * restrict s, uint8_t i)
{
	if (s->se_type == SAVEDSEL && s->se_index == jbxvt.scr.sline.max - i) {
		show_selection(0, cheight - 1, 0, cwidth - 1);
		s->se_type = NOSEL;
	}
}

// Save lines that scroll of the top of the screen.
static void save_top_lines(uint8_t count)
{
	for (uint8_t i = 1; i <= count; i++) {
		save_top_line(i);
		save_top_lines_show_selection(&jbxvt.sel.end1, i);
		save_top_lines_show_selection(&jbxvt.sel.end2, i);
	}
}

static void set_selend_index(uint8_t i, uint8_t count, struct selst * restrict s)
{
	if (s->se_type == SAVEDSEL && s->se_index == i)
		  s->se_index = i + count;
}

/*  Scroll count lines from row1 to row2 inclusive.  row1 should be <= row2.
 *  scrolling is up for a +ve count and down for a -ve count.
 *  count is limited to a maximum of MAX_SCROLL lines.
 */
void scroll(int16_t row1, int16_t row2, int8_t count)
{
	int y1, y2;
	unsigned char *save[MAX_SCROLL], *rend[MAX_SCROLL];
	struct slinest *sl;


	if (row1 == 0 && screen == &screen1 && count > 0) {
		save_top_lines(count);
		for (int16_t i = jbxvt.scr.sline.max - count - 1; i >= 0; i--) {
			jbxvt.scr.sline.data[i + count] = jbxvt.scr.sline.data[i];
			set_selend_index(i, count, &jbxvt.sel.end1);
			set_selend_index(i, count, &jbxvt.sel.end2);
		}
		for (uint8_t i = 0; i < count; i++) {
			unsigned char * s = screen->text[i];
			unsigned char * r = screen->rend[i];
			int16_t j;
			for (j = cwidth - 1; j >= 0 && s[j] == 0; j--)
				;
			j++;
			sl = (struct slinest *)malloc(sizeof(struct slinest));
			sl->sl_text = (unsigned char *)malloc(j + 1);
			memcpy(sl->sl_text,s,j);
			sl->sl_text[j] = s[cwidth];
			if (r[cwidth] == 0)
				sl->sl_rend = NULL;
			else {
				sl->sl_rend = (unsigned char *)malloc(j + 1);
				memcpy(sl->sl_rend,r,j);
			}
			sl->sl_length = j;
			jbxvt.scr.sline.data[count - i - 1] = sl;
			sel_scr_to_sav(&jbxvt.sel.end1, i, count);
			sel_scr_to_sav(&jbxvt.sel.end2, i, count);
			}
		jbxvt.scr.sline.top += count;
		if (jbxvt.scr.sline.top > jbxvt.scr.sline.max)
			jbxvt.scr.sline.top = jbxvt.scr.sline.max;
		sbar_show(cheight + jbxvt.scr.sline.top - 1, jbxvt.scr.offset,
			jbxvt.scr.offset + cheight - 1);
	}

	if (count > 0) {
		int16_t j = row1;
		for (uint8_t i = 0; i < count; i++, j++) {
			save[i] = screen->text[j];
			rend[i] = screen->rend[j];
			if (jbxvt.sel.end1.se_type == SCREENSEL && jbxvt.sel.end1.se_index == j) {
				show_selection(0,cheight - 1,0,cwidth - 1);
				jbxvt.sel.end1.se_type = NOSEL;
			}
			if (jbxvt.sel.end2.se_type == SCREENSEL && jbxvt.sel.end2.se_index == j) {
				show_selection(0,cheight - 1,0,cwidth - 1);
				jbxvt.sel.end2.se_type = NOSEL;
			}
		}
		row2++;
		for (; j < row2; j++) {
			screen->text[j - count] = screen->text[j];
			screen->rend[j - count] = screen->rend[j];
			if (jbxvt.sel.end1.se_type == SCREENSEL && jbxvt.sel.end1.se_index == j)
				jbxvt.sel.end1.se_index = j - count;
			if (jbxvt.sel.end2.se_type == SCREENSEL && jbxvt.sel.end2.se_index == j)
				jbxvt.sel.end2.se_index = j - count;
		}
		for (uint8_t i = 0; i < count; i++) {
			memset(save[i],0,cwidth + 1);
			screen->text[row2 - i - 1] = save[i];
			memset(rend[i],0,cwidth + 1);
			screen->rend[row2 - i - 1] = rend[i];
		}
		if (count < row2 - row1) {
			y2 = MARGIN + row1 * jbxvt.X.font_height;
			y1 = y2 + count * jbxvt.X.font_height;
			uint16_t height = (row2 - row1 - count) * jbxvt.X.font_height;
			XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.win.vt,jbxvt.X.gc.tx,
				0,y1,pwidth,height,0,y2);
			repair_damage();
		}
		uint16_t height = count * jbxvt.X.font_height;
		y1 = MARGIN + (row2 - count) * jbxvt.X.font_height;
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,0,y1,pwidth,height,False);
		return;
	}
	if (count < 0) {
		count = -count;
		int16_t j = row2 - 1;
		for (uint8_t i = 0; i < count; i++, j--) {
			save[i] = screen->text[j];
			rend[i] = screen->rend[j];
			if (jbxvt.sel.end1.se_type == SCREENSEL && jbxvt.sel.end1.se_index == j) {
				show_selection(0,cheight - 1,0,cwidth - 1);
				jbxvt.sel.end1.se_type = NOSEL;
			}
			if (jbxvt.sel.end2.se_type == SCREENSEL && jbxvt.sel.end2.se_index == j) {
				show_selection(0,cheight - 1,0,cwidth - 1);
				jbxvt.sel.end2.se_type = NOSEL;
			}
		}
		for (; j >= row1; j--) {
			screen->text[j + count] = screen->text[j];
			screen->rend[j + count] = screen->rend[j];
			if (jbxvt.sel.end1.se_type == SCREENSEL && jbxvt.sel.end1.se_index == j)
				jbxvt.sel.end1.se_index = j + count;
			if (jbxvt.sel.end2.se_type == SCREENSEL && jbxvt.sel.end2.se_index == j)
				jbxvt.sel.end2.se_index = j + count;
		}
		for (uint8_t i = 0; i < count; i++) {
			memset(save[i],0,cwidth + 1);
			screen->text[row1 + i] = save[i];
			memset(rend[i],0,cwidth + 1);
			screen->rend[row1 + i] = rend[i];
		}
		if (count < row2 - row1) {
			y1 = MARGIN + row1 * jbxvt.X.font_height;
			y2 = y1 + count * jbxvt.X.font_height;
			uint16_t height = (row2 - row1 - count) * jbxvt.X.font_height;
			XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.win.vt,jbxvt.X.gc.tx,
				0,y1,pwidth,height,0,y2);
			repair_damage();
		}
		uint16_t height = count * jbxvt.X.font_height;
		y1 = MARGIN + row1 * jbxvt.X.font_height;
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,0,y1,pwidth,height,False);
	}
}


static uint8_t get_count(const uint8_t count, uint8_t * restrict n)
{
	/*  If count is greater than MAX_SCROLL then scroll in
	 *  installements.
	 */
	*n = count > MAX_SCROLL ? MAX_SCROLL : count;
	return count - *n;
}

/*  Scroll screen1 up by count lines saving lines as needed.  This is used
 *  after the screen size is reduced.
 */
void scroll1(uint8_t count)
{
//	int j;
	uint8_t n;
//	int16_t i;
	unsigned char *save[MAX_SCROLL], *rend[MAX_SCROLL];
	struct slinest *sl;
	unsigned char *r, *s;

	while (count > 0) {
		count=get_count(count, &n);
		/*  Save lines that scroll of the top of the screen.
		 */
		for (uint8_t i = 1; i <= n; i++) {
			if ((sl = jbxvt.scr.sline.data[jbxvt.scr.sline.max - i]) != NULL) {
				free(sl->sl_text);
				if (sl->sl_rend != NULL)
					free(sl->sl_rend);
				free((void *)sl);
			}
		}
		for (int16_t i = jbxvt.scr.sline.max - n - 1; i >= 0; i--) {
			jbxvt.scr.sline.data[i + n] = jbxvt.scr.sline.data[i];
		}
		for (uint8_t i = 0; i < n; i++) {
			s = screen1.text[i];
			r = screen1.rend[i];
			int16_t j;
			for (j = cwidth - 1; j >= 0 && s[j] == 0; j--)
				;
			j++;
			sl = (struct slinest *)malloc(sizeof(struct slinest));
			sl->sl_text = (unsigned char *)malloc(j + 1);
			memcpy(sl->sl_text,s,j);
			sl->sl_text[j] = s[cwidth];
			if (r[cwidth] == 0)
				sl->sl_rend = NULL;
			else {
				sl->sl_rend = (unsigned char *)malloc(j + 1);
				memcpy(sl->sl_rend,r,j);
			}
			sl->sl_length = j;
			jbxvt.scr.sline.data[n - i - 1] = sl;
		}
		jbxvt.scr.sline.top += n;
		if (jbxvt.scr.sline.top > jbxvt.scr.sline.max)
			jbxvt.scr.sline.top = jbxvt.scr.sline.max;

		uint16_t j = 0;
		for (uint8_t i = 0; i < n; i++, j++) {
			save[i] = screen1.text[j];
			rend[i] = screen1.rend[j];
		}
		for (; j < cheight; j++) {
			screen1.text[j - n] = screen1.text[j];
			screen1.rend[j - n] = screen1.rend[j];
		}
		for (uint8_t i = 0; i < n; i++) {
			memset(save[i],0,cwidth + 1);
			screen1.text[cheight - i - 1] = save[i];
			memset(rend[i],0,cwidth + 1);
			screen1.rend[cheight - i - 1] = rend[i];
		}
	}
}


