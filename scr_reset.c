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
static void clear_window(xcb_connection_t * xc)
{
    const struct JBDim p = jbxvt_get_pixel_size();
    xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), 0, 0, p.w, p.h);
}
static void decscnm(xcb_connection_t * xc)
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
void jbxvt_reset(xcb_connection_t * xc)
{
    LOG("jbxvt_reset()");
        jbxvt_zero_rstyle();
    jbxvt_resize_window(xc);
    jbxvt_repaint(xc);
    struct JBDim csz = jbxvt_get_char_size();
    jbxvt_set_tty_size(csz);
    jbxvt_draw_scrollbar(xc);
    decscnm(xc);
    jbxvt_get_margin()->bottom = csz.height - 1;
    jbxvt_repaint(xc);
    jbxvt_draw_cursor(xc);
}
