/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_reset.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_move.h"
#include "screen.h"
#include "scroll.h"

#include <unistd.h>

// Shortcuts
#define S jbxvt.scr
#define S0 S.s[0]
#define S1 S.s[1]
#define P S.pixels
#define X jbxvt.X

static void init_screen_elements(struct JBXVTScreen * restrict scr)
{
	scr->margin.bottom = S.chars.height - 1;
	scr->wrap_next = false;
	scr->margin.top = 0;
}

static void init(struct JBXVTScreen * s)
{
	for (size_t y = 0; y < JBXVT_MAX_ROWS; ++y) {
		s->text[y] = calloc(JBXVT_MAX_COLS, 1);
		s->rend[y] = calloc(JBXVT_MAX_COLS, 4);
	}
}

static inline void fix_margins(const struct JBDim c)
{
	/* On screen resize, check if old margin was on the bottom line.
	   If so, set the bottom margin to the new bottom line.  */
	if (c.height == S.chars.height)
		  return;
	if (SCR->margin.b >= c.h)
		  SCR->margin.b = c.h - 1;
}

static void decscnm(void)
{
	static bool last_was_rv;
	const bool rv = jbxvt.mode.decscnm;
	if (last_was_rv == rv) // Already has either mode set
		return;
	else
		last_was_rv = rv;
	LOG("decscnm()");
	struct JBXVTXPixels * p = &X.color;
	JB_SWAP(pixel_t, p->fg, p->bg);
	JB_SWAP(pixel_t, p->current_fg, p->current_bg);
	xcb_change_gc(X.xcb, X.gc.tx, XCB_GC_FOREGROUND
		| XCB_GC_BACKGROUND, (uint32_t[]){p->fg, p->bg});
	xcb_change_window_attributes(X.xcb, X.win.vt,
		XCB_CW_BACK_PIXEL, &p->bg);
	usleep(100000);
}

/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void scr_reset(void)
{
	LOG("scr_reset()");
	decscnm();
	struct JBDim c = jbxvt_get_char_size(P);
	fix_margins(c);
	static bool created;
	if (!created) {
		init(&jbxvt.scr.s[0]);
		init(&jbxvt.scr.s[1]);
		created = true;
	}
	int16_t * y = &SCR->cursor.y;
	if (likely(SCR == &S.s[0]) && *y >= c.h) {
		scroll1(*y - c.h + 1);
		*y = c.h - 1;
	}
	init_screen_elements(&S.s[0]);
	init_screen_elements(&S.s[1]);
	// Constrain dimensions:
	c.w = MIN(c.w, JBXVT_MAX_COLS);
	c.h = MIN(c.h, JBXVT_MAX_ROWS);
	tty_set_size(c);
	S.chars = c;
	reset_row_col();
	--c.h; --c.w;
	sbar_draw(c.h + S.sline.top, S.offset,
		S.offset + c.h);
	decscnm();
	xcb_flush(X.xcb);
	repaint();
	draw_cursor();
}

