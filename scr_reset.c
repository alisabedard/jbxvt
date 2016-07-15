/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_reset.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_move.h"
#include "selection.h"
#include "ttyinit.h"

#include <gc.h>
#include <string.h>

static void init_screen_elements(VTScreen * restrict scr,
	uint8_t ** restrict text, uint32_t ** restrict rend)
{
	scr->margin.bottom = jbxvt.scr.chars.height - 1;
	scr->decom = scr->wrap_next = false;
	scr->rend = rend;
	scr->text = text;
	scr->margin.top = 0;
}

__attribute__((pure))
static Size get_cdim(const Size d)
{
	const uint8_t m = MARGIN<<1;
	const Size f = jbxvt.X.font_size;
	return (Size){.width = (d.w-m)/f.w,
		.height = (d.h-m)/f.h};
}

static void init(void)
{
	uint8_t **s1, **s2;
	uint32_t **r1, **r2;
	uint16_t sz = JBXVT_MAX_ROWS * sizeof(void *);
	s1 = GC_MALLOC(sz);
	s2 = GC_MALLOC(sz);
	r1 = GC_MALLOC(sz);
	r2 = GC_MALLOC(sz);
	for (int_fast16_t y = JBXVT_MAX_ROWS - 1; y >= 0; --y) {
		sz = JBXVT_MAX_COLS;
		s1[y] = GC_MALLOC(sz);
		s2[y] = GC_MALLOC(sz);
		sz <<= 2;
		r1[y] = GC_MALLOC(sz);
		r2[y] = GC_MALLOC(sz);
	}
	jbxvt.scr.s1.text = s1;
	jbxvt.scr.s2.text = s2;
	jbxvt.scr.s1.rend = r1;
	jbxvt.scr.s2.rend = r2;
}

static inline void fix_margins(const Size c)
{
	/* On screen resize, check if old margin was on the bottom line.
	   If so, set the bottom margin to the new bottom line.  */
	if (c.height == jbxvt.scr.chars.height)
		  return;
	VTScreen * restrict s = jbxvt.scr.current;
	if (s->margin.b >= c.h)
		  s->margin.b = c.h - 1;
}

/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void scr_reset(void)
{
	LOG("scr_reset()");
	Size c = get_cdim(jbxvt.scr.pixels);
	fix_margins(c);
	static bool created;
	if (!created) {
		init();
		created = true;
	}
	uint8_t **s1 = jbxvt.scr.s1.text, **s2 = jbxvt.scr.s2.text;
	uint32_t **r1 = jbxvt.scr.s1.rend, **r2 = jbxvt.scr.s2.rend;
	VTScreen * scr = jbxvt.scr.current;
	if (likely(scr == &jbxvt.scr.s1 && jbxvt.scr.s1.text)
		&& scr->cursor.y >= c.h) {
		scroll1(scr->cursor.y - c.h + 1);
		scr->cursor.y = c.h - 1;
	}
	init_screen_elements(&jbxvt.scr.s1, s1, r1);
	init_screen_elements(&jbxvt.scr.s2, s2, r2);
	scr_start_selection((xcb_point_t){}, SEL_CHAR);
	// Constrain dimensions:
	c.w = MIN(c.w, JBXVT_MAX_COLS);
	c.h = MIN(c.h, JBXVT_MAX_ROWS);
	tty_set_size(c.w, c.h);
	jbxvt.scr.chars = c;
	reset_row_col();
	--c.h; --c.w;
	sbar_show(c.h + jbxvt.scr.sline.top, jbxvt.scr.offset,
		jbxvt.scr.offset + c.h);
	repaint();
	cursor(CURSOR_DRAW);
}

