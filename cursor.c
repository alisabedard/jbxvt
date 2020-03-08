/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "cursor.h"
#include "JBXVTPrivateModes.h"
#include "font.h"
#include "gc.h"
#include "libjb/macros.h"
#include "mode.h"
#include "repaint.h"
#include "sbar.h"
#include "size.h"
#include "window.h"
static uint32_t saved_style;
static struct JBDim saved_cursor;
static uint8_t cursor_attr = JBXVT_DEFAULT_CURSOR_ATTR;
extern inline struct JBDim jbxvt_get_cursor(void);
extern inline int16_t jbxvt_get_x(void);
extern inline int16_t jbxvt_get_y(void);
void jbxvt_blink_cursor(xcb_connection_t * xc)
{
    if (!jbxvt_get_modes()->att610 && cursor_attr % 2) {
        jbxvt_draw_cursor(xc); // blinking cursor
        xcb_flush(xc);
    }
}
// Ensure cursor coordinates are valid per screen and decom mode
// Returns new cursor y value
int16_t jbxvt_check_cursor_position(void)
{
    struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
    struct JBDim * restrict c = &s->cursor;
    { // sz scope
        struct JBDim sz = jbxvt_get_char_size();
        --sz.w; --sz.h;
        { // m scope
            struct JBDim * restrict m = &s->margin;
            m->top = JB_MAX(m->top, 0); // Sanitize margins
            m->bottom = JB_MIN(m->bottom, sz.h);
            if (jbxvt_get_modes()->decom) // Implement DECOM
                JB_LIMIT(c->y, m->top, m->bottom);
        }
        JB_LIMIT(c->x, sz.w, 0);
        JB_LIMIT(c->y, sz.h, 0);
    }
    return c->y;
}
void jbxvt_set_cursor_attr(const uint8_t val)
{
    if (val <= 8) // sanitize max (see below)
        cursor_attr = val;
}
void jbxvt_save_cursor(void)
{
    saved_cursor = jbxvt_get_current_screen()->cursor;
    saved_style = jbxvt_get_rstyle();
}
void jbxvt_restore_cursor(xcb_connection_t * xc)
{
    jbxvt_draw_cursor(xc);
    jbxvt_get_current_screen()->cursor = saved_cursor;
    jbxvt_zero_rstyle();
    jbxvt_add_rstyle(saved_style);
    jbxvt_draw_cursor(xc);
}
static inline bool is_blinking(void)
{
    return cursor_attr && cursor_attr % 2;
}
void jbxvt_draw_cursor(xcb_connection_t * xc)
{
    // Don't draw if scrolled, non-existent, or hidden
    struct JBXVTScreen * current;
    if (jbxvt_get_scroll() || !(current = jbxvt_get_current_screen())
        || !jbxvt_get_modes()->dectcem)
        return;
    if ((current->cursor_visible ^= true) && is_blinking())
        jbxvt_repaint(xc); // prevent stale cursor blocks
    struct JBDim p = jbxvt_chars_to_pixels(current->cursor);
    const struct JBDim f = jbxvt_get_font_size();
    xcb_rectangle_t r = {p.x, p.y, f.w, f.h};
    switch (cursor_attr) {
    case 0: // blinking block
    case 1: // blinking block
    case 2: // steady block (default)
        break;
    case 3: // blinking underline
    case 4: // steady underline
        r.height = 2;
        r.y += f.h - 2;
        break;
    case 5: // blinking bar
    case 6: // steady bar
        r.width = 2;
        break;
    case 7: // blinking overline
    case 8: // steady overline
        r.height = 2;
        break;
    }
    xcb_poly_fill_rectangle(xc, jbxvt_get_vt_window(xc),
        jbxvt_get_cursor_gc(xc), 1, &r);
}
