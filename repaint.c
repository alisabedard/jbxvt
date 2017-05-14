/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "repaint.h"
#include "JBXVTScreen.h"
#include "config.h"
#include "font.h"
#include "gc.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "show_selection.h"
#include "size.h"
#include "window.h"
static uint_fast16_t get_render_length(const rstyle_t * rvec,
	const uint16_t len)
{
	uint_fast16_t i = 0;
	while (i < len && rvec[i] == rvec[0])
		++i;
	return i;
}
struct RenderToken {
	uint8_t * string;
	rstyle_t * style;
	struct JBDim position;
	uint16_t length;
	const bool is_double_width_line;
};
// Display the string described by the RenderToken structure
static void paint_RenderToken(xcb_connection_t * xc, struct RenderToken *
	restrict token)
{
	const uint8_t font_width = jbxvt_get_font_size().width;
	for (int i; token->length; token->length -= i, token->string += i,
		token->style += i, token->position.x += i * font_width)
		jbxvt_paint(xc, token->string, *token->style,
			i = get_render_length(token->style, token->length),
			token->position, token->is_double_width_line);
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
	paint_RenderToken(xc, &(struct RenderToken){
		.string = filter(l->text, w), .style =
		l->rend, .length = w, .position = position,
		.is_double_width_line = l->dwl});
}
struct HistoryIterator {
	xcb_connection_t * xc;
	struct JBDim * position;
	struct JBXVTLine * line_data;
	int line, top;
	uint16_t scroll_size;
	uint8_t font_height, char_height;
};
static int show_history_r(struct HistoryIterator * restrict i)
{
	if (i->scroll_size == 0)
		i->scroll_size = jbxvt_get_scroll_size();
	// This is the normal return condition of this recursive function:
	if (i->top < 0)
		return i->line;
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
	i->position->y += i->font_height;
	++i->line;
	--i->top;
	return show_history_r(i);
}
static void draw_history_line(xcb_connection_t * xc, const int16_t y)
{
	const uint16_t width = jbxvt_get_pixel_size().width;
	xcb_point_t onscreen_line[] = {{.y = y}, {.x = width, .y = y}};
	xcb_poly_line(xc, XCB_COORD_MODE_ORIGIN, jbxvt_get_vt_window(xc),
		jbxvt_get_cursor_gc(xc), 2, onscreen_line);
}
static void draw_on_screen_lines(xcb_connection_t * xc, struct JBDim *
	restrict position, int line, const int char_height)
{
	struct JBXVTScreen * s = jbxvt_get_current_screen();
	const uint8_t font_height = jbxvt_get_font_size().height;
	for (uint_fast16_t i = 0; line <= char_height;
		++line, ++i, position->y += font_height)
		paint(xc, s->line + i, *position);
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
	const int line = show_history_r(&(struct HistoryIterator){.xc = xc,
		.position = &p, .font_height = jbxvt_get_font_size().height,
		.char_height = chars.height, .top = jbxvt_get_scroll() - 1});
	// Save the position where scroll history ends:
	const int16_t history_end_y = p.y - 1;
	// Do the remainder from the current screen:
	draw_on_screen_lines(xc, &p, line, chars.height);
	if (history_end_y > 0)
		draw_history_line(xc, history_end_y);
	jbxvt_show_selection(xc);
}
