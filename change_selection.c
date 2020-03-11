/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "change_selection.h"
#include "font.h"
#include "gc.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "selection.h"
#include "selend.h"
#include "size.h"
#include "window.h"
struct InversionData {
    xcb_connection_t * xc;
    struct JBDim font_size, char_size, start_point, end_point;
    xcb_window_t window;
    xcb_gcontext_t gc;
};
static void invert_r(const uint8_t current,
    struct InversionData * i)
{
    LOG("invert_r(current: %d, i)", current);
    if (current <= i->end_point.row) {
        const uint8_t fw = (uint8_t)i->font_size.width,
              fh = (uint8_t)i->font_size.height;
        const int16_t x[] = {current == i->start_point.row ?
            i->start_point.columns * fw : 0,
            (current == i->end_point.row ? i->end_point.columns
             : i->char_size.width) * fw};
        xcb_poly_fill_rectangle(i->xc, i->window, i->gc, 1,
            &(xcb_rectangle_t){.x = x[0], .y = current * fh,
            .width = (uint16_t)(x[1] - x[0]), .height = fh});
        invert_r(current + 1, i);
    }
}
static void change(xcb_connection_t * xc, struct JBDim * se,
    struct JBDim * ose)
{
    LOG("change()");
    const int16_t n = jbxvt_selcmp(se, ose);
    if (!n) // nothing selected
        return;
    // repaint the start.
    struct JBDim start, end;
    { // nn scope
        const bool nn = n < 0;
        start = jbxvt_get_selend_position(nn ? se : ose);
        end = jbxvt_get_selend_position(nn ? ose : se);
    }
    struct InversionData i = {.xc = xc,
        .font_size = jbxvt_get_font_size(),
        .char_size = jbxvt_get_char_size(),
        .start_point = start, .end_point = end,
        .window = jbxvt_get_vt_window(xc),
        .gc = jbxvt_get_cursor_gc(xc),
    };
    // Invert the changed area
    invert_r((uint8_t)i.start_point.row, &i);

}
/*  Repaint the displayed selection to reflect the new value.
    ose1 and ose2 are assumed to represent the currently
    displayed selection endpoints.  */
void jbxvt_change_selection(xcb_connection_t * xc,
    struct JBDim * ose0, struct JBDim * ose1)
{
    LOG("jbxvt_change_selection()");
    if (jbxvt_selcmp(ose0, ose1) > 0) {
        struct JBDim * se = ose0;
        ose0 = ose1;
        ose1 = se;
    }
    struct JBDim * e = jbxvt_order_selection_ends(
        jbxvt_get_selection_end_points());
    change(xc, e, ose0);
    change(xc, e + 1, ose1);
}
