/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "scr_reset.h"
#include "command.h"
#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/time.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_move.h"
#include "screen.h"
#include "scroll.h"
#include <string.h>
#include <unistd.h>
static void init_screen_elements(struct JBXVTScreen * restrict scr)
{
	scr->margin.bottom = jbxvt.scr.chars.height - 1;
	scr->wrap_next = false;
	scr->margin.top = 0;
}
static void init(struct JBXVTScreen * s)
{
	for (size_t y = 0; y < JBXVT_MAX_ROWS; ++y) {
		s->text[y] = calloc(1, JBXVT_MAX_COLS);
		s->rend[y] = calloc(4, JBXVT_MAX_COLS);
	}
}
static inline void fix_margins(const struct JBDim c)
{
	/* On screen resize, check if old margin was on the bottom line.
	   If so, set the bottom margin to the new bottom line.  */
	if (c.height == jbxvt.scr.chars.height)
		  return;
	if (jbxvt.scr.current->margin.b >= c.h)
		  jbxvt.scr.current->margin.b = c.h - 1;
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
	struct JBXVTXPixels * p = &jbxvt.X.color;
	JB_SWAP(pixel_t, p->fg, p->bg);
	JB_SWAP(pixel_t, p->current_fg, p->current_bg);
	xcb_change_gc(jbxvt.X.xcb, jbxvt.X.gc.tx, XCB_GC_FOREGROUND
		| XCB_GC_BACKGROUND, (uint32_t[]){p->fg, p->bg});
	xcb_change_window_attributes(jbxvt.X.xcb, jbxvt.X.win.vt,
		XCB_CW_BACK_PIXEL, &p->bg);
	jb_sleep(100);
}
/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void jbxvt_reset(void)
{
	LOG("jbxvt_reset()");
	decscnm();
	struct JBDim c = jbxvt.scr.chars;
	fix_margins(c);
	static bool created;
	if (!created) {
		init(&jbxvt.scr.s[0]);
		init(&jbxvt.scr.s[1]);
		created = true;
	}
	int16_t * y = &jbxvt.scr.current->cursor.y;
	if (JB_LIKELY(jbxvt.scr.current == &jbxvt.scr.s[0]) && *y >= c.h) {
		jbxvt_scroll_primary_screen(*y - c.h + 1);
		*y = c.h - 1;
	}
	init_screen_elements(&jbxvt.scr.s[0]);
	init_screen_elements(&jbxvt.scr.s[1]);
	// Constrain dimensions:
	c.w = JB_MIN(c.w, JBXVT_MAX_COLS);
	c.h = JB_MIN(c.h, JBXVT_MAX_ROWS);
	jbxvt_set_tty_size(c);
	jbxvt.scr.chars = c;
	reset_row_col();
	--c.h; --c.w;
	jbxvt_draw_scrollbar();
	decscnm();
	xcb_flush(jbxvt.X.xcb);
	jbxvt_repaint();
	jbxvt_draw_cursor();
}
