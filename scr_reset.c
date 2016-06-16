/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_reset.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "sbar.h"
#include "selection.h"
#include "ttyinit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME:  There is a memory leak when screen size changes.
   Attempts to fix by freeing previous larger size causes
   segmentation fault.  */
static void free_old(void)
{
	for(uint_fast8_t y = 0; y < jbxvt.scr.chars.height; ++y) {
		size_t l = 0;
		while(jbxvt.scr.s1.text[y][l++])
			  ;
		if (l == 1)
			  continue;
		l = 0;
		while(jbxvt.scr.s2.text[y][l++])
			  ;
		if (l == 1)
			  continue;

		free(jbxvt.scr.s1.text[y]);
		free(jbxvt.scr.s2.text[y]);
		free(jbxvt.scr.s1.rend[y]);
		free(jbxvt.scr.s2.rend[y]);
	}
	if (jbxvt.scr.s1.text) {
		free(jbxvt.scr.s1.text);
		free(jbxvt.scr.s2.rend);
	}
	if (jbxvt.scr.s2.text) {
		free(jbxvt.scr.s2.text);
		free(jbxvt.scr.s1.rend);
	}
}

void reset_row_col(void)
{
	xcb_point_t * c = &jbxvt.scr.current->cursor;
	xcb_point_t p = *c;
	int16_t l = jbxvt.scr.chars.width - 1;
	c->x = p.x < 0 ? 0 : p.x > l ? l : p.x;
	l = jbxvt.scr.chars.height - 1;
	c->y = p.y < 0 ? 0 : p.y > l ? l : p.y;
	// Implement DECOM, DEC Origin Mode, limits
	if (jbxvt.scr.current->decom) {
		const Size m = jbxvt.scr.current->margin;
		if (c->y < m.top) {
			c->y = m.top;
		} else if (c->y > m.bottom) {
			c->y = m.bottom;
		}
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

static Size get_dim(void)
{
	xcb_get_geometry_reply_t * r = xcb_get_geometry_reply(jbxvt.X.xcb,
		xcb_get_geometry(jbxvt.X.xcb, jbxvt.X.win.vt), NULL);
	if (!r)
		  return (Size){0};
	Size s = {.w = r->width, .h = r->height};
	free(r);
	return s;
}

__attribute__((pure))
static Size get_cdim(const Size d)
{
	const uint8_t m = MARGIN<<1;
	return (Size){.width = (d.w-m)/jbxvt.X.font_width,
		.height = (d.h-m)/jbxvt.X.font_height};
}

static void cpl(struct screenst * restrict scr, uint8_t ** restrict s,
	uint32_t ** restrict r, const uint8_t i, const uint8_t j,
	const uint16_t sz) // copy line
{
	// copy contents:
	if(!s[j] || !r[j])
		return;
	memcpy(s[j], scr->text[i], sz);
	memcpy(r[j], scr->rend[i], sz<<2);
	// copy end byte for wrap flag:
	s[j][sz] = scr->text[i][jbxvt.scr.chars.width];
}

static int save_data_on_screen(uint8_t cw, int i, const int j,
	bool * restrict onscreen, uint8_t ** restrict s1,
	uint32_t ** restrict r1, uint8_t ** restrict s2,
	uint32_t ** restrict r2)
{
	// truncate to fit:
	const uint16_t n = cw > jbxvt.scr.chars.width
	      ?	jbxvt.scr.chars.width : cw;
	// copy contents:
	cpl(&jbxvt.scr.s1, s1, r1, i, j, n);
	cpl(&jbxvt.scr.s1, s2, r2, i, j, n);
	if (--i < 0) {
		*onscreen = false;
		return 0;
	}
		*onscreen = false;
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
	if (!sl) // prevent segfault.
		  return i;
	const uint8_t l = sl->sl_length;
	if (!l || !sl->sl_text)
		  return i + 1;
	const uint8_t n = cw < l ? cw : l;
	memcpy(s1[j], sl->sl_text, n);
	if (sl->sl_rend) {
		memcpy(r1[j], sl->sl_rend, n<<2);
	}
	return i + 1;
}

/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void scr_reset(void)
{
	Size d = get_dim();
	Size c = get_cdim(d);
	if (!jbxvt.scr.current->text || c.w != jbxvt.scr.chars.width
		|| c.h != jbxvt.scr.chars.height) {
		uint8_t **s1, **s2;
		uint32_t **r1, **r2;
		jbxvt.scr.offset = 0;
		/*  Recreate the screen backup arrays.
		 *  The screen arrays are one word wider than the screen and
		 *  the last word is used as a flag which is non-zero if the
		 *  line wrapped automatically.
		 */
		const uint8_t rowsz = c.h * sizeof(void*);
		s1 = malloc(rowsz);
		s2 = malloc(rowsz);
		r1 = malloc(rowsz);
		r2 = malloc(rowsz);
		for (uint16_t y = 0; y < c.h; ++y) {
			uint8_t w = c.w + 1;
			s1[y] = calloc(w, 1);
			s2[y] = calloc(w, 1);
			r1[y] = calloc(--w, 4);
			r2[y] = calloc(w, 4);
		}
		if (jbxvt.scr.s1.text) {
			if (jbxvt.scr.s1.cursor.y >= c.h)
				  jbxvt.scr.s1.cursor.y = c.h - 1;
			// calculate working no. of lines.
			int16_t i = jbxvt.scr.sline.top
				+ jbxvt.scr.s1.cursor.y + 1;
			int32_t j = i > c.h ? c.h - 1 : i - 1;
			i = jbxvt.scr.s1.cursor.y; // save
			jbxvt.scr.s1.cursor.y = j;
			bool onscreen = true;
			for (; j >= 0; j--)
				  i = onscreen ? save_data_on_screen(c.w, i,
					  j, &onscreen, s1, r1, s2, r2)
					  : handle_offscreen_data(c.w, i, j,
						  s1, r1);
			if (onscreen) // avoid segfault
				  return;
				  //abort();
			for (j = i; j < jbxvt.scr.sline.top; ++j) {
				if (!jbxvt.scr.sline.data[j])
					  break;
				  jbxvt.scr.sline.data[j - i]
					  = jbxvt.scr.sline.data[j];
			}
			jbxvt.scr.sline.top -= i;
			free_old();
		}
		jbxvt.scr.chars = c;
		jbxvt.scr.pixels = d;
		init_screen_elements(&jbxvt.scr.s1, s1, r1);
		init_screen_elements(&jbxvt.scr.s2, s2, r2);
		scr_start_selection((xcb_point_t){},CHAR);
	}
	tty_set_size(c.w, c.h);
	reset_row_col();
	c.h--;
	c.w--;
	sbar_show(c.h + jbxvt.scr.sline.top, jbxvt.scr.offset,
		jbxvt.scr.offset + c.h);
	repaint((xcb_point_t){}, (xcb_point_t){.y = c.h, .x = c.w});
	cursor(CURSOR_DRAW);
}

