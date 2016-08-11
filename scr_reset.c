/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_reset.h"

#include "command.h"
#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "repaint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_move.h"
#include "selection.h"

#include <gc.h>
#include <string.h>

static void init_screen_elements(VTScreen * restrict scr,
	uint8_t ** restrict text, uint32_t ** restrict rend)
{
	scr->margin.bottom = jbxvt.scr.chars.height - 1;
	scr->wrap_next = false;
	scr->rend = rend;
	scr->text = text;
	scr->margin.top = 0;
}

__attribute__((pure))
static Size get_cdim(const Size d)
{
	const Size f = jbxvt.X.f.size;
	// Take advantage of division rounding down for mod
	return (Size) {.w = (d.w - MARGIN)/f.w,
		.h = (d.h - MARGIN)/f.h};
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
	jbxvt.scr.s[0].text = s1;
	jbxvt.scr.s[1].text = s2;
	jbxvt.scr.s[0].rend = r1;
	jbxvt.scr.s[1].rend = r2;
	jbxvt.scr.s[0].wrap = GC_MALLOC(JBXVT_MAX_ROWS);
	jbxvt.scr.s[1].wrap = GC_MALLOC(JBXVT_MAX_ROWS);
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

static void clear(void)
{
	struct JBXVTXData * x = &jbxvt.X;
	struct JBXVTScreenData * scr = &jbxvt.scr;
	const Size sz = scr->pixels;
	xcb_clear_area(x->xcb, false, x->win.vt, 0, 0, sz.w, sz.h<<1);
	xcb_flush(x->xcb);
	change_offset(0);
	VTScreen * s = scr->current;
	const Size c = scr->chars;
	for (int i = s->cursor.y; i < c.height; ++i)
		for (int j = i == s->cursor.y ? s->cursor.x : 0;
			i < c.w; ++i) {
			s->text[i][j] = 0;
			s->rend[i][j] = 0;
		}
}

static void decscnm(void)
{
	static bool last_was_rv;
	const bool rv = jbxvt.mode.decscnm;
	if (last_was_rv == rv) // Already has either mode set
		return;
	struct JBXVTXPixels * p = &jbxvt.X.color;
	SWAP(pixel_t, p->fg, p->bg);
	SWAP(pixel_t, p->current_fg, p->current_bg);
	xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FOREGROUND
		| XCB_GC_BACKGROUND, (uint32_t[]){p->fg, p->bg});
	xcb_change_window_attributes(jbxvt.X.xcb, jbxvt.X.win.vt,
		XCB_CW_BACK_PIXEL, &p->bg);
	xcb_flush(jbxvt.X.xcb);
}

/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void scr_reset(void)
{
	LOG("scr_reset()");
	decscnm();
	Size c = get_cdim(jbxvt.scr.pixels);
	fix_margins(c);
	static bool created;
	if (!created) {
		init();
		created = true;
	}
	uint8_t **s1 = jbxvt.scr.s[0].text, **s2 = jbxvt.scr.s[1].text;
	uint32_t **r1 = jbxvt.scr.s[0].rend, **r2 = jbxvt.scr.s[1].rend;
	VTScreen * scr = jbxvt.scr.current;
	if (likely(scr == &jbxvt.scr.s[0] && jbxvt.scr.s[0].text)
		&& scr->cursor.y >= c.h) {
		scroll1(scr->cursor.y - c.h + 1);
		scr->cursor.y = c.h - 1;
	}
	init_screen_elements(&jbxvt.scr.s[0], s1, r1);
	init_screen_elements(&jbxvt.scr.s[1], s2, r2);
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
	clear();
	decscnm();
	repaint();
	draw_cursor();
}

