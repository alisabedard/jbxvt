#include "scroll.h"

#include "jbxvt.h"
#include "repair_damage.h"
#include "sbar.h"
#include "screen.h"
#include "show_selection.h"
#include "xvt.h"

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif//DEBUG

static void sel_scr_to_sav(struct selst * restrict s,
	const int i, const int count)
{
	if (s->se_type == SCREENSEL && s->se_index == i) {
		s->se_type = SAVEDSEL;
		s->se_index = count - i - 1;
	}
}

static void save_top_line(const int i)
{
	struct slinest ** s = &jbxvt.scr.sline.data[jbxvt.scr.sline.max - i];
	if(*s) {
		if((*s)->sl_rend) {
			free((*s)->sl_rend);
			(*s)->sl_rend = NULL;
		}
		free(*s);
		*s = NULL;
	}
}

static void save_top_lines_show_selection(struct selst * restrict s,
	const int i)
{
	if (s->se_type == SAVEDSEL
		&& s->se_index == jbxvt.scr.sline.max - i) {
		show_selection(0, jbxvt.scr.chars.height - 1, 0,
			jbxvt.scr.chars.width - 1);
		s->se_type = NOSEL;
	}
}

// Save lines that scroll of the top of the screen.
static void save_top_lines(int count)
{
	for (int i = 1; i <= count; i++) {
		save_top_line(i);
		save_top_lines_show_selection(&jbxvt.sel.end1, i);
		save_top_lines_show_selection(&jbxvt.sel.end2, i);
	}
}

static void set_selend_index(const int i, const int count,
	struct selst * restrict s)
{
	if (s->se_type == SAVEDSEL && s->se_index == i)
		  s->se_index = i + count;
}

static void scroll_up(int row1, int row2, int count)
{
#ifdef DEBUG
	printf("count: %d, row1: %d, row2: %d\n", count, row1, row2);
#endif//DEBUG
	if (row1 == 0 && jbxvt.scr.current == &jbxvt.scr.s1) {
		save_top_lines(count);
		for (int i = jbxvt.scr.sline.max - count - 1;
			i >= 0; i--) {
			jbxvt.scr.sline.data[i + count]
				= jbxvt.scr.sline.data[i];
			set_selend_index(i, count, &jbxvt.sel.end1);
			set_selend_index(i, count, &jbxvt.sel.end2);
		}
		for (int i = 0; i < count; i++) {
			unsigned char * s = jbxvt.scr.current->text[i];
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
	unsigned char *save[MAX_SCROLL];
	uint32_t *rend[MAX_SCROLL];
	for (int i = 0; i < count; i++, j++) {
		save[i] = jbxvt.scr.current->text[j];
		rend[i] = jbxvt.scr.current->rend[j];
		if (jbxvt.sel.end1.se_type == SCREENSEL
			&& jbxvt.sel.end1.se_index == j) {
			show_selection(0,jbxvt.scr.chars.height - 1,
				0, jbxvt.scr.chars.width - 1);
			jbxvt.sel.end1.se_type = NOSEL;
		}
		if (jbxvt.sel.end2.se_type == SCREENSEL
			&& jbxvt.sel.end2.se_index == j) {
			show_selection(0,jbxvt.scr.chars.height - 1,
				0, jbxvt.scr.chars.width - 1);
			jbxvt.sel.end2.se_type = NOSEL;
		}
	}
	row2++;
	for (; j < row2; j++) {
		jbxvt.scr.current->text[j - count]
			= jbxvt.scr.current->text[j];
		jbxvt.scr.current->rend[j - count]
			= jbxvt.scr.current->rend[j];
		if (jbxvt.sel.end1.se_type == SCREENSEL
			&& jbxvt.sel.end1.se_index == j)
			  jbxvt.sel.end1.se_index = j - count;
		if (jbxvt.sel.end2.se_type == SCREENSEL
			&& jbxvt.sel.end2.se_index == j)
			  jbxvt.sel.end2.se_index = j - count;
	}
	for (int i = 0; i < count; i++) {
		memset(save[i],0,jbxvt.scr.chars.width + 1);
		jbxvt.scr.current->text[row2 - i - 1] = save[i];
		memset(rend[i],0,(jbxvt.scr.chars.width + 1)*sizeof(uint32_t));
		jbxvt.scr.current->rend[row2 - i - 1] = rend[i];
	}
	int y1;
	if (count < row2 - row1) {
		const int y2 = MARGIN + row1
			* jbxvt.X.font_height;
		y1 = y2 + count * jbxvt.X.font_height;
		int height = (row2 - row1 - count)
			* jbxvt.X.font_height;
		XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,
			jbxvt.X.win.vt,jbxvt.X.gc.tx,
			0,y1,jbxvt.scr.pixels.width,height,0,y2);
		repair_damage();
	}
	int height = count * jbxvt.X.font_height;
	y1 = MARGIN + (row2 - count) * jbxvt.X.font_height;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
		0,y1,jbxvt.scr.pixels.width,height,False);
}

static void scroll_down(int row1, int row2, int count)
{
	count = -count;
	int j = row2 - 1;
	uint32_t *rend[MAX_SCROLL];
	unsigned char *save[MAX_SCROLL];
	for (int i = 0; i < count; i++, j--) {
		save[i] = jbxvt.scr.current->text[j];
		rend[i] = jbxvt.scr.current->rend[j];
		if (jbxvt.sel.end1.se_type == SCREENSEL
			&& jbxvt.sel.end1.se_index == j) {
			show_selection(0,jbxvt.scr.chars.height - 1,
				0,jbxvt.scr.chars.width - 1);
			jbxvt.sel.end1.se_type = NOSEL;
		}
		if (jbxvt.sel.end2.se_type == SCREENSEL
			&& jbxvt.sel.end2.se_index == j) {
			show_selection(0,jbxvt.scr.chars.height - 1,
				0,jbxvt.scr.chars.width - 1);
			jbxvt.sel.end2.se_type = NOSEL;
		}
	}
	for (; j >= row1; j--) {
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
	for (int i = 0; i < count; i++) {
		memset(save[i],0,jbxvt.scr.chars.width + 1);
		jbxvt.scr.current->text[row1 + i] = save[i];
		memset(rend[i],0,jbxvt.scr.chars.width + 1);
		jbxvt.scr.current->rend[row1 + i] = rend[i];
	}
	int y1, y2;
	if (count < row2 - row1) {
		y1 = MARGIN + row1 * jbxvt.X.font_height;
		y2 = y1 + count * jbxvt.X.font_height;
		int height = (row2 - row1 - count)
			* jbxvt.X.font_height;
		XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,
			jbxvt.X.win.vt,jbxvt.X.gc.tx,
			0,y1,jbxvt.scr.pixels.width,height,0,y2);
		repair_damage();
	}
	int height = count * jbxvt.X.font_height;
	y1 = MARGIN + row1 * jbxvt.X.font_height;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,0,y1,
		jbxvt.scr.pixels.width,height,False);
}

/*  Scroll count lines from row1 to row2 inclusive.  row1 should be <= row2.
 *  scrolling is up for a +ve count and down for a -ve count.
 *  count is limited to a maximum of MAX_SCROLL lines.
 */
void scroll(const int row1, const int row2, const int count)
{
	if(count<0) {
		scroll_down(row1, row2, count);
		return;
	}
	scroll_up(row1, row2, count);
}

static int get_count(const int count, int * restrict n)
{
	/*  If count is greater than MAX_SCROLL then scroll in
	 *  installements.
	 */
	*n = count > MAX_SCROLL ? MAX_SCROLL : count;
	return count - *n;
}

/*  Scroll jbxvt.scr.s1 up by count lines saving lines as needed.  This is used
 *  after the screen size is reduced.
 */
void scroll1(int count) // unsigned as only going up
{
	int n;
	unsigned char *save[MAX_SCROLL];
	uint32_t *rend[MAX_SCROLL];
	struct slinest *sl;
	unsigned char *s;

	while (count > 0) {
		count=get_count(count, &n);
		//  Save lines that scroll of the top of the screen.
		for (int i = 1; i <= n; i++) {
			sl = jbxvt.scr.sline.data[jbxvt.scr.sline.max - i];
			if(!sl) continue;
			free(sl->sl_text);
			if(sl->sl_rend)
				  free(sl->sl_rend);
			free(sl);
		}
		for (int i = jbxvt.scr.sline.max - n - 1; i >= 0; i--) {
			jbxvt.scr.sline.data[i + n] = jbxvt.scr.sline.data[i];
		}
		for (int i = 0; i < n; i++) {
			s = jbxvt.scr.s1.text[i];
			uint32_t * r = jbxvt.scr.s1.rend[i];
			int j;
			for (j = jbxvt.scr.chars.width - 1;
				j >= 0 && s[j] == 0; j--)
				;
			j++;
			sl = (struct slinest *)malloc(sizeof(struct slinest));
			sl->sl_text = (unsigned char *)malloc(j + 1);
			memcpy(sl->sl_text,s,j);
			sl->sl_text[j] = s[jbxvt.scr.chars.width];
			if (r[jbxvt.scr.chars.width] == 0)
				sl->sl_rend = NULL;
			else {
				sl->sl_rend = malloc((j + 1)
					*sizeof(uint32_t));
				memcpy(sl->sl_rend,r,(j*sizeof(uint32_t)));
			}
			sl->sl_length = j;
			jbxvt.scr.sline.data[n - i - 1] = sl;
		}
		jbxvt.scr.sline.top += n;
		if (jbxvt.scr.sline.top > jbxvt.scr.sline.max)
			jbxvt.scr.sline.top = jbxvt.scr.sline.max;

		int j = 0;
		for (int i = 0; i < n; i++, j++) {
			save[i] = jbxvt.scr.s1.text[j];
			rend[i] = jbxvt.scr.s1.rend[j];
		}
		for (; j < jbxvt.scr.chars.height; j++) {
			jbxvt.scr.s1.text[j - n] = jbxvt.scr.s1.text[j];
			jbxvt.scr.s1.rend[j - n] = jbxvt.scr.s1.rend[j];
		}
		for (int i = 0; i < n; i++) {
			memset(save[i],0,jbxvt.scr.chars.width + 1);
			jbxvt.scr.s1.text[jbxvt.scr.chars.height - i - 1]
				= save[i];
			memset(rend[i],0,jbxvt.scr.chars.width + 1);
			jbxvt.scr.s1.rend[jbxvt.scr.chars.height - i - 1]
				= rend[i];
		}
	}
}

