// Copyright 2017, Jeffrey E. Bedard
#undef DEBUG
#include "move.h"
#include <stdbool.h>
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
#include "cursor.h"
#include "libjb/JBDim.h"
#ifdef DEBUG
#include "libjb/log.h"
#endif//DEBUG
#include "libjb/macros.h"
#include "mode.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "size.h"
static inline int16_t dim(const int16_t cursor,
    const int16_t delta, const bool relative)
{
    // Sanitize non-relative arguments--must be positive.
    return relative ? cursor + delta : JB_MAX(delta, 0);
}
#ifdef DEBUG
static void debug_move(int16_t x, int16_t y, uint8_t relative)
{
#define BSTR(v, f) (v & f ? 'T' : 'F')
#define RSTR(r) BSTR(relative, JBXVT_##r##_RELATIVE)
    LOG("jbxvt_move(x: %d, y: %d, row: %c, col: %c)", x,
        y, RSTR(ROW), RSTR(COLUMN));
}
#else//!DEBUG
#define debug_move(x, y, r)
#endif//DEBUG
/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void jbxvt_move(xcb_connection_t * xc,
    const int16_t x, const int16_t y, const uint8_t relative)
{
    debug_move(x, y, relative);
    jbxvt_set_scroll(xc, 0);
    jbxvt_draw_cursor(xc); // clear
    struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
    { // c scope
        struct JBDim c = s->cursor;
        { // cr, rr scope
            const bool cr = relative & JBXVT_COLUMN_RELATIVE,
                  rr = relative & JBXVT_ROW_RELATIVE;
            s->cursor = c = (struct JBDim) {
                .x = dim(c.x, x, cr),
                .y = dim(c.y, y, rr)};
        }
        c.y = jbxvt_check_cursor_position();
        jbxvt_check_selection(xc, c.y, c.y);
    }
    s->wrap_next = false;
    jbxvt_draw_cursor(xc); // draw
}
