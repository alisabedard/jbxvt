/*  Copyright 2017, Jeffrey E. Bedard */
/* #undef DEBUG */
#include "move.h"
#include "cursor.h"
#include "screen.h"
/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void jbxvt_move(xcb_connection_t * xc,
    const int16_t x, const int16_t y, const uint8_t relative)
{
    struct JBXVTScreen * s;
    s = jbxvt_get_current_screen();
    jbxvt_draw_cursor(xc);
    s->cursor.x = relative & JBXVT_COLUMN_RELATIVE ? s->cursor.x + x : x;
    s->cursor.y = relative & JBXVT_ROW_RELATIVE ? s->cursor.y + y : y;
    s->wrap_next = false;
    jbxvt_draw_cursor(xc);
}
