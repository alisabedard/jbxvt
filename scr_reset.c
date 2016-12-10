/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
//#undef DEBUG
#include "scr_reset.h"
#include <string.h>
#include <unistd.h>
#include "color.h"
#include "command.h"
#include "config.h"
#include "cursor.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "libjb/time.h"
#include "mode.h"
#include "paint.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_move.h"
#include "screen.h"
#include "scroll.h"
#include "size.h"
#include "window.h"
static void init_screen_elements(struct JBXVTScreen * restrict scr)
{
	scr->margin.bottom = jbxvt_get_char_size().height - 1;
	scr->wrap_next = false;
	scr->margin.top = 0;
}
static inline void fix_margins(const struct JBDim c)
{
	/* On screen resize, check if old margin was on the bottom line.
	   If so, set the bottom margin to the new bottom line.  */
	if (c.height == jbxvt_get_char_size().height)
		  return;
	if (jbxvt_get_margin()->bottom >= c.h)
		  jbxvt_get_margin()->bottom = c.h - 1;
}
static void clear_window(xcb_connection_t * restrict xc)
{
	const struct JBDim p = jbxvt_get_pixel_size();
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), 0, 0, p.w, p.h);
}
static void decscnm(xcb_connection_t * restrict xc)
{
	static bool last_was_rv;
	const bool rv = jbxvt_get_modes()->decscnm;
	if (last_was_rv == rv) // Already has either mode set
		return;
	else
		last_was_rv = rv;
	LOG("decscnm()");
	jbxvt_reverse_screen_colors(xc);
	/* Clear area here to make visual bell effect in vi more apparent.  */
	clear_window(xc);
	xcb_flush(xc);
	jb_sleep(111);
}
/*  Reset the screen - called whenever the screen
    needs to be repaired completely.  */
void jbxvt_reset(xcb_connection_t * restrict xc)
{
	LOG("jbxvt_reset()");
	struct JBDim c = jbxvt_get_char_size();
	fix_margins(c);
	init_screen_elements(jbxvt_get_screen_at(0));
	init_screen_elements(jbxvt_get_screen_at(1));
	// Constrain dimensions:
	c.w = JB_MIN(c.w, JBXVT_MAX_COLUMNS);
	c.h = JB_MIN(c.h, JBXVT_MAX_ROWS);
	jbxvt_set_tty_size(c);
	jbxvt_set_pixel_size(jbxvt_chars_to_pixels(c));
	jbxvt_check_cursor_position();
	jbxvt_draw_scrollbar(xc);
	decscnm(xc);
	jbxvt_repaint(xc);
	jbxvt_draw_cursor(xc);
}
