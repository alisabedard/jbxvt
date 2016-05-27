/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_reset.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "sbar.h"
#include "scroll_up.h"
#include "selection.h"
#include "ttyinit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME:  There is a memory leak when screen size changes.
   Attempts to fix by freeing previous larger size causes
   segmentation fault.  */
static void free_visible_screens(uint8_t ch)
{
	LOG("free_visible_screens(%d), sch: %d", ch,
		jbxvt.scr.chars.height);
	// Avoid segfault when screen size changes:
	ch=ch>jbxvt.scr.chars.height?jbxvt.scr.chars.height:ch;
	for(uint8_t y = 0; y < ch; y++) {
		LOG("y:%d of sch:%d\tch:%d", y, jbxvt.scr.chars.height, ch);
		free(jbxvt.scr.s1.text[y]);
		free(jbxvt.scr.s2.text[y]);
		if(jbxvt.scr.s1.rend[y])
			free(jbxvt.scr.s1.rend[y]);
		if(jbxvt.scr.s2.rend[y])
			free(jbxvt.scr.s2.rend[y]);
	}
	free(jbxvt.scr.s1.text);
	free(jbxvt.scr.s2.text);
	free(jbxvt.scr.s1.rend);
	free(jbxvt.scr.s2.rend);
}

void reset_row_col(void)
{
	jbxvt.scr.current->cursor.col = constrain(jbxvt.scr.current->cursor.col,
		jbxvt.scr.chars.width);
	jbxvt.scr.current->cursor.row = constrain(jbxvt.scr.current->cursor.row,
		jbxvt.scr.chars.height);
}

static void fill_and_scroll(const uint8_t ch)
{
	/*  Now fill up the screen from the old screen
	    and saved lines.  */
	if (jbxvt.scr.s1.cursor.row >= ch) {
		// scroll up to save any lines that will be lost.
		scroll_up(jbxvt.scr.s1.cursor.row - ch + 1);
		jbxvt.scr.s1.cursor.row = ch - 1;
	}
}

static void init_screen_elements(struct screenst * restrict scr,
	uint8_t ** restrict text, uint32_t ** restrict rend)
{
	scr->margin.bottom = jbxvt.scr.chars.height - 1;
	scr->decom = false;
	scr->rend = rend;
	scr->text = text;
	scr->margin.top = 0;
	scr->wrap_next = false;
}

static Dim get_dim(void)
{
	Window dw;
	int d;
	unsigned int w, h, u;

	XGetGeometry(jbxvt.X.dpy, jbxvt.X.win.vt, &dw, &d, &d,
		&w, &h, &u, &u);

	return (Dim){.width = w, .height = h};
}

__attribute__((pure))
static Dim get_cdim(const Dim d)
{
	const uint8_t m = MARGIN<<1;
	return (Dim){.width = (d.w-m)/jbxvt.X.font_width,
		.height = (d.h-m)/jbxvt.X.font_height};
}

static void cpl(struct screenst * restrict scr, uint8_t ** restrict s,
	uint32_t ** restrict r, const uint8_t i, const uint8_t j,
	const uint8_t sz) // copy line
{
	// copy contents:
	memcpy(s[j], scr->text[i], sz);
	memcpy(r[j], scr->rend[i], sz * sizeof(uint32_t));
	// copy end byte for wrap flag:
	s[j][sz] = scr->text[i][jbxvt.scr.chars.width];
	r[j][sz] = scr->rend[i][jbxvt.scr.chars.width];
}

static int save_data_on_screen(uint8_t cw, int i, const int j,
	bool * restrict onscreen, uint8_t ** restrict s1,
	uint32_t ** restrict r1, uint8_t ** restrict s2,
	uint32_t ** restrict r2)
{
	// truncate to fit:
	uint8_t n = constrain(cw, jbxvt.scr.chars.width + 1);
	// copy contents:
	cpl(&jbxvt.scr.s1, s1, r1, i, j, n);
	cpl(&jbxvt.scr.s1, s2, r2, i, j, n);
	i--;
	if (i < 0) {
		*onscreen = false;
		i = 0;
	}
	return i;
}

static int handle_offscreen_data(const uint8_t cw,
	const int i, const int j,
	uint8_t ** restrict s1,
	uint32_t ** restrict r1)
{
	if (i >= jbxvt.scr.sline.top)
		  return i;
	struct slinest *sl = jbxvt.scr.sline.data[i];
	const uint8_t n = constrain(cw, sl->sl_length + 1);
	memcpy(s1[j],sl->sl_text,n);
	free(sl->sl_text);
	sl->sl_text=NULL;
	if (sl->sl_rend) {
		memcpy(r1[j],sl->sl_rend, n*sizeof(uint32_t));
		r1[j][cw] = sl->sl_rend [sl->sl_length];
		free(sl->sl_rend);
	}
	free(sl);
	return i + 1;
}

/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void scr_reset(void)
{
	Dim d = get_dim();
	Dim c = get_cdim(d);
	if (!jbxvt.scr.current->text || c.w != jbxvt.scr.chars.width
		|| c.h != jbxvt.scr.chars.height) {
		uint8_t **s1, **s2;
		uint32_t **r1, **r2;
		jbxvt.scr.offset = 0;
		/*  Recreate the screen backup arrays.
		 *  The screen arrays are one byte wider than the screen and
		 *  the last byte is used as a flag which is non-zero of the
		 *  line wrapped automatically.
		 */
		s1 = malloc(c.h * sizeof(void*));
		s2 = malloc(c.h * sizeof(void*));
		r1 = malloc(c.h * sizeof(uint32_t*));
		r2 = malloc(c.h * sizeof(uint32_t*));
		for (uint16_t y = 0; y < c.h; y++) {
			const uint8_t w = c.w + 1;
			s1[y] = calloc(w, 1);
			s2[y] = calloc(w, 1);
			r1[y] = calloc(w, sizeof(uint32_t));
			r2[y] = calloc(w, sizeof(uint32_t));
		}
		if (jbxvt.scr.s1.text) {
			fill_and_scroll(c.h);
			// calculate working no. of lines.
			int16_t i = jbxvt.scr.sline.top
				+ jbxvt.scr.s1.cursor.row + 1;
			int16_t j = i > c.h ? c.h - 1 : i - 1;
			i = jbxvt.scr.s1.cursor.row; // save
			jbxvt.scr.s1.cursor.row = j;
			bool onscreen = true;
			for (; j >= 0; j--)
				  i = onscreen ? save_data_on_screen(c.w, i,
					  j, &onscreen, s1, r1, s2, r2)
					  : handle_offscreen_data(c.w, i, j,
						  s1, r1);
#ifdef DEBUG
			if (onscreen)
				  abort();
#endif//DEBUG
			for (j = i; j < jbxvt.scr.sline.top; j++)
				  jbxvt.scr.sline.data[j - i]
					  = jbxvt.scr.sline.data[j];
			for (j = jbxvt.scr.sline.top - i;
				j < jbxvt.scr.sline.top; j++)
				  jbxvt.scr.sline.data[j] = NULL;
			jbxvt.scr.sline.top -= i;
			free_visible_screens(c.h);
		}
		jbxvt.scr.chars = c;
		jbxvt.scr.pixels = d;
		init_screen_elements(&jbxvt.scr.s1, s1, r1);
		init_screen_elements(&jbxvt.scr.s2, s2, r2);
		scr_start_selection((Dim){},CHAR);
	}
	tty_set_size(c.w, c.h);
	reset_row_col();
	c.h--;
	c.w--;
	sbar_show(c.h + jbxvt.scr.sline.top, jbxvt.scr.offset,
		jbxvt.scr.offset + c.h);
	repaint((Dim){}, c);
	cursor(CURSOR_DRAW);
}

