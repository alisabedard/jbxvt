#include "scr_reset.h"

#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "sbar.h"
#include "scroll.h"
#include "selection.h"
#include "ttyinit.h"
#include "xvt.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_visible_screens(uint8_t ch)
{
	LOG("free_visible_screens(%d)", ch);
	ch=ch<jbxvt.scr.chars.height?ch:jbxvt.scr.chars.height;
	//for(uint8_t y = 0; y < jbxvt.scr.chars.height; y++) {
	for(uint8_t y = 0; y < ch; y++) {
		LOG("y:%d of sch:%d\tch:%d", y,
			jbxvt.scr.chars.height, ch);
		assert(jbxvt.scr.s1.text[y]);
		free(jbxvt.scr.s1.text[y]);

		assert(jbxvt.scr.s2.text[y]);
		free(jbxvt.scr.s2.text[y]);

		assert(jbxvt.scr.s1.rend[y]);
		free(jbxvt.scr.s1.rend[y]);

		assert(jbxvt.scr.s2.rend[y]);
		free(jbxvt.scr.s2.rend[y]);
	}
	assert(jbxvt.scr.s1.text);
	free(jbxvt.scr.s1.text);

	assert(jbxvt.scr.s2.text);
	free(jbxvt.scr.s2.text);

	assert(jbxvt.scr.s1.rend);
	free(jbxvt.scr.s1.rend);

	assert(jbxvt.scr.s2.rend);
	free(jbxvt.scr.s2.rend);
}

static void reset_row_col(void)
{
	if (jbxvt.scr.current->col >= jbxvt.scr.chars.width)
		jbxvt.scr.current->col = jbxvt.scr.chars.width - 1;
	if (jbxvt.scr.current->row >= jbxvt.scr.chars.height)
		jbxvt.scr.current->row = jbxvt.scr.chars.height - 1;
}

static void fill_and_scroll(const uint8_t ch)
{
	/*  Now fill up the screen from the old screen
	    and saved lines.  */
	if (jbxvt.scr.s1.row >= ch) {
		// scroll up to save any lines that will be lost.
		scroll1(jbxvt.scr.s1.row - ch + 1);
		jbxvt.scr.s1.row = ch - 1;
	}
}

static void get_cwh(uint8_t * restrict cw, uint8_t * restrict ch,
	uint16_t * restrict pw, uint16_t * restrict ph)
{
	int d;
	unsigned int w, h, u;
	Window dw;

	XGetGeometry(jbxvt.X.dpy, jbxvt.X.win.vt, &dw, &d, &d,
		&w, &h, &u, &u);
	const uint8_t m = MARGIN * 2;
	*cw = (w-m)/jbxvt.X.font_width;
	*ch = (h-m)/jbxvt.X.font_height;
	*pw = w;
	*ph = h;
}

/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void scr_reset(void)
{
	uint8_t cw, ch;
	uint16_t width, height;
	get_cwh(&cw, &ch, &width, &height);
	if (!jbxvt.scr.current->text || cw != jbxvt.scr.chars.width
		|| ch != jbxvt.scr.chars.height) {
		unsigned char **s1, **s2;
		uint32_t **r1, **r2;
		jbxvt.scr.offset = 0;
		/*  Recreate the screen backup arrays.
		 *  The screen arrays are one byte wider than the screen and
		 *  the last byte is used as a flag which is non-zero of the
		 *  line wrapped automatically.
		 */
		s1 = (unsigned char **)malloc(ch * sizeof(void*));
		s2 = (unsigned char **)malloc(ch * sizeof(void*));
		r1 = malloc(ch * sizeof(void*));
		r2 = malloc(ch * sizeof(void*));
		for (uint16_t y = 0; y < ch; y++) {
			s1[y] = calloc(cw + 1, 1);
			s2[y] = calloc(cw + 1, 1);
			r1[y] = calloc(cw + 1, sizeof(uint32_t));
			r2[y] = calloc(cw + 1, sizeof(uint32_t));
		}
		if (jbxvt.scr.s1.text) {
			fill_and_scroll(ch);
			// calculate working no. of lines.
			int i = jbxvt.scr.sline.top + jbxvt.scr.s1.row + 1;
			int j = i > ch ? ch - 1 : i - 1;
			i = jbxvt.scr.s1.row;
			jbxvt.scr.s1.row = j;
			bool onscreen = true;
			for (; j >= 0; j--) {
				int n;
				if (onscreen) {
					n = cw < jbxvt.scr.chars.width
						? cw : jbxvt.scr.chars.width;
					memcpy(s1[j],jbxvt.scr.s1.text[i],n);
					memcpy(s2[j],jbxvt.scr.s2.text[i],n);
					memcpy(r1[j],jbxvt.scr.s1.rend[i],
						n * sizeof(uint32_t));
					memcpy(r2[j],jbxvt.scr.s2.rend[i],
						n * sizeof(uint32_t));
					s1[j][cw]
						= jbxvt.scr.s1.text[i]
						[jbxvt.scr.chars.width];
					s2[j][cw]
						= jbxvt.scr.s2.text[i]
						[jbxvt.scr.chars.width];
					r1[j][cw]
						= jbxvt.scr.s1.rend[i]
						[jbxvt.scr.chars.width];
					r2[j][cw]
						= jbxvt.scr.s2.rend[i]
						[jbxvt.scr.chars.width];
					i--;
					if (i < 0) {
						onscreen = 0;
						i = 0;
					}
				} else {
					if (i >= jbxvt.scr.sline.top)
						  break;
					struct slinest *sl
						= jbxvt.scr.sline.data[i];
					n = cw < sl->sl_length
						? cw : sl->sl_length;
					memcpy(s1[j],sl->sl_text,n);
					free(sl->sl_text);
					if (sl->sl_rend) {
						memcpy(r1[j],sl->sl_rend,
							n*sizeof(uint32_t));
						r1[j][cw] =
							sl->sl_rend
							[sl->sl_length];
						free(sl->sl_rend);
						sl->sl_rend=NULL;
					}
					free((void *)sl);
					i++;
				}
			}
			if (onscreen)
				  abort();
			for (j = i; j < jbxvt.scr.sline.top; j++)
				  jbxvt.scr.sline.data[j - i]
					  = jbxvt.scr.sline.data[j];
			for (j = jbxvt.scr.sline.top - i;
				j < jbxvt.scr.sline.top; j++)
				  jbxvt.scr.sline.data[j] = NULL;
			jbxvt.scr.sline.top -= i;
			free_visible_screens(ch);
		}

		jbxvt.scr.chars.width = cw;
		jbxvt.scr.chars.height = ch;
		jbxvt.scr.pixels.width = width;
		jbxvt.scr.pixels.height = height;

                jbxvt.scr.s1.bmargin = jbxvt.scr.chars.height - 1;
		jbxvt.scr.s1.decom = false;
		jbxvt.scr.s1.rend = r1;
		jbxvt.scr.s1.text = s1;
		jbxvt.scr.s1.tmargin = 0;
		jbxvt.scr.s1.wrap_next = false;

                jbxvt.scr.s2.bmargin = jbxvt.scr.chars.height - 1;
		jbxvt.scr.s2.decom = false;
		jbxvt.scr.s2.rend = r2;
		jbxvt.scr.s2.text = s2;
		jbxvt.scr.s2.tmargin = 0;
		jbxvt.scr.s2.wrap_next = false;

		scr_start_selection(0,0,CHAR);
	}
	tty_set_size(jbxvt.scr.chars.width,jbxvt.scr.chars.height);
	reset_row_col();
	sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
		jbxvt.scr.offset, jbxvt.scr.offset
		+ jbxvt.scr.chars.height - 1);
	repaint(0,jbxvt.scr.chars.height - 1,0,jbxvt.scr.chars.width - 1);
	cursor();
}

