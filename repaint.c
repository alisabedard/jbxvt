/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "repaint.h"
#include "JBXVTPaintContext.h"
#include "JBXVTScreen.h"
#include "config.h"
#include "font.h"
#include "gc.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "show_selection.h"
#include "size.h"
#include "window.h"
static int get_length_of_style(register int i,
	struct JBXVTPaintContext * restrict token)
{
	const int length = token->length;
	const rstyle_t style = token->style[0];
	while (i < length && token->style[i] == style)
		++i;
	return i;
}
// Display the string described by the JBXVTPaintContext structure
static void paint_JBXVTPaintContext(struct JBXVTPaintContext * restrict token)
{
	const uint8_t font_width = jbxvt_get_font_size().width;
	for (int i; token->length > 0; i = get_length_of_style(0, token),
		token->length -= i, token->string += i,
		token->style += i, token->position.x += i * font_width)
		jbxvt_paint(token);
}
static uint8_t * filter(uint8_t * restrict t, register int_fast16_t i)
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
	int32_t line, top, scroll_size;
	uint16_t char_height, font_height;
};
static void increment_line(struct HistoryContext * restrict i)
{
	i->position->y += i->font_height;
	++i->line;
	--i->top;
}
static void show_history_r(struct HistoryContext * restrict i)
{
	if (i->scroll_size == 0)
		i->scroll_size = jbxvt_get_scroll_size();
	// This is the normal return condition of this recursive function:
	if (i->top < 0)
		return;
	/* Use i->top as the offset into the scroll history buffer, read from
	 * bottom to top.  So we use the value of jbxvt_get_scroll_size() to
	 * find the bottom of the buffer.  Then offset by -1 to base the index
	 * on 0.  */
	if (!i->line_data) // initialize once:
		i->line_data = jbxvt_get_saved_lines() + i->scroll_size -
			i->top - 1;
	else // avoid function call and math overhead:
		++i->line_data;
	paint(i->xc, i->line_data, *i->position);
	increment_line(i);
	return show_history_r(i);
}
static void draw_history_line(xcb_connection_t * xc, const int16_t y)
{
	const int width = jbxvt_get_pixel_size().width;
	xcb_point_t onscreen_line[] = {{.y = y}, {.x = width, .y = y}};
	xcb_poly_line(xc, XCB_COORD_MODE_ORIGIN, jbxvt_get_vt_window(xc),
		jbxvt_get_cursor_gc(xc), 2, onscreen_line);
}
// Display current lines after the last rendered scroll history line (i):
static void draw_active_lines_r(int i, struct HistoryContext * c)
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
	// Subtract 1 from scroll offset to get index.
	struct HistoryContext history = {.xc = xc,
		.position = &p, .font_height = jbxvt_get_font_size().height,
		.char_height = chars.height, .top = jbxvt_get_scroll() - 1};
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
