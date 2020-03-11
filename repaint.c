/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "repaint.h"
#include "JBXVTPaintContext.h"
#include "JBXVTScreen.h"
#include "config.h"
#include "font.h"
#include "gc.h"
#include "libjb/log.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "show_selection.h"
#include "size.h"
#include "window.h"
static int get_length_of_style_r(int i, struct JBXVTPaintContext * restrict t)
{
    return i < t->length && t->style[i] == t->style[0]
        ? get_length_of_style_r(i + 1, t) : i;
}
// Display the string described by the JBXVTPaintContext structure
static void paint_JBXVTPaintContext(struct JBXVTPaintContext * restrict token)
{
    const uint8_t font_width = jbxvt_get_font_size().width;
    for (int i; token->length > 0; i = get_length_of_style_r(0, token),
        token->length -= i, token->string += i,
        token->style += i, token->position.x += i * font_width)
        jbxvt_paint(token);
}
/* t is returned to allow chain calling.  Leave this iterative since this is
 * called very frequently.   */
static uint8_t * filter(uint8_t * restrict t, register int i)
{
    while (--i >= 0)
        if (t[i] < ' ')
            t[i] = ' ';
    return t;
}
static void paint(xcb_connection_t * xc, struct JBXVTLine * l,
    const struct JBDim position)
{
    const uint16_t w = jbxvt_get_char_size().width;
    paint_JBXVTPaintContext(&(struct JBXVTPaintContext){ .xc = xc,
        .string = filter(l->text, w), .style = l->rend, .length = w,
        .position = position, .is_double_width_line = l->dwl});
}
struct HistoryContext {
    xcb_connection_t * xc;
    struct JBDim * position;
    struct JBXVTLine * line_data;
    int16_t line, top;
    uint16_t char_height, font_height;
};
static struct JBXVTLine * get_line(struct HistoryContext * restrict i)
{
    /* Use i->top as the offset into the scroll history buffer, read from
     * bottom to top.  So we use the value of jbxvt_get_scroll_size() to
     * find the bottom of the buffer.  Then offset by -1 to base the index
     * on 0.  */
    return jbxvt_get_saved_lines() + jbxvt_get_scroll_size() - i->top - 1;
}
static void show_history_r(struct HistoryContext * restrict i)
{
    if (i->top > 0) { // show remaining lines in history
        i->line_data = get_line(i);
        paint(i->xc, i->line_data, *i->position);
        i->position->y += i->font_height;
        ++i->line;
        --i->top; // one less line
        show_history_r(i);
    }
}
// Draw a line to indicate the end of the scroll history.
static void draw_history_line(xcb_connection_t * xc, const int16_t y)
{
    const int width = jbxvt_get_pixel_size().width;
    xcb_point_t onscreen_line[] = {{.y = y}, {.x = width, .y = y}};
    xcb_poly_line(xc, XCB_COORD_MODE_ORIGIN, jbxvt_get_vt_window(xc),
        jbxvt_get_cursor_gc(xc), 2, onscreen_line);
}
// Display current lines after the last rendered scroll history line (i):
static void draw_active_lines_r(const int i, struct HistoryContext * c)
{
    if (i <= c->char_height) {
        paint(c->xc, c->line_data + i, *c->position);
        c->position->y += c->font_height;
        draw_active_lines_r(i + 1, c);
    }
}
// Repaint the screen
void jbxvt_repaint(xcb_connection_t * xc)
{
    // First do any 'scrolled off' lines that are visible.
    const struct JBDim chars = jbxvt_get_char_size();
    if (chars.rows >= JBXVT_MAX_ROWS)
        return; // invalid screen size, go no further.
    struct JBDim p = {{0},{0}};
    const struct JBDim f = jbxvt_get_font_size();
    struct HistoryContext history = {.xc = xc,
        .position = &p, .font_height = f.height,
        .char_height = chars.height,
        // Subtract 1 from scroll offset to get index.
        .top = jbxvt_get_scroll() - 1};
#ifdef JBXVT_DEBUG_HISTORY
    LOG("history.top: %d", history.top);
#endif // JBXVT_DEBUG_HISTORY
    show_history_r(&history);
    // Save the position where scroll history ends:
    const int16_t history_end_y = p.y - 1;
    // Do the remainder from the current screen:
    history.line_data = jbxvt_get_current_screen()->line;
    draw_active_lines_r(history.line, &history);
    if (history_end_y > 0)
        draw_history_line(xc, history_end_y);
    jbxvt_show_selection(xc);
}
