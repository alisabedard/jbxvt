/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
//#undef DEBUG
#include "scr_reset.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
#include "color.h"
#include "command.h"
#include "config.h"
#include "cursor.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "libjb/time.h"
#include "mode.h"
#include "move.h"
#include "repaint.h"
#include "sbar.h"
#include "screen.h"
#include "size.h"
#include "window.h"
static inline void fix_margins(const struct JBDim c)
{
	/* On screen resize, check if old margin was on the bottom line.
	   If so, set the bottom margin to the new bottom line.  */
	jbxvt_get_margin()->bottom = c.height - 1;
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
	jbxvt_resize_window(xc);
	struct JBDim c = jbxvt_get_char_size();
	fix_margins(c);
	jbxvt_set_tty_size(c);
	jbxvt_check_cursor_position();
	jbxvt_draw_scrollbar(xc);
	decscnm(xc);
	jbxvt_repaint(xc);
	jbxvt_draw_cursor(xc);
	xcb_flush(xc);
}
