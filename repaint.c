/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "repaint.h"
#include "JBXVTScreen.h"
#include "config.h"
#include "font.h"
#include "gc.h"
#include "libjb/JBDim.h"
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
// Display the string using the render vector at the screen
// coordinates.
static void paint_rvec_text(xcb_connection_t * restrict xc,
	uint8_t * restrict str, rstyle_t * restrict rvec, uint16_t len,
	struct JBDim p, const bool dwl)
{
	struct RenderToken t = {.string = str, .style = rvec, .length = len,
		.position = p, .is_double_width_line = dwl};
	paint_RenderToken(xc, &t);
}
static uint8_t * filter(uint8_t * restrict t, register int_fast16_t i)
{
	while (--i >= 0)
		if (t[i] < ' ')
			t[i] = ' ';
	return t;
}
static void paint(xcb_connection_t * xc, struct JBXVTLine * l,
	const struct JBDim p)
{
	const uint16_t w = jbxvt_get_char_size().width;
	paint_rvec_text(xc, filter(l->text, w), l->rend, w, p, l->dwl);
}
static int show_history(xcb_connection_t * restrict xc, const int line,
	const int top, struct JBDim * restrict p, const uint8_t font_height,
	const struct JBDim char_size)
{
	const uint16_t ss = jbxvt_get_scroll_size();
	const uint8_t h = char_size.height;
	/* Check  top + h vs ss so that the following pointer
	 * arithmetic does not go outside array bounds.  */
	if (top > ss)
		return top;
	/* This is the normal return condition of this recursive
	 * function:  */
	if (line >= h || top < 0)
		return line;
	/* Use screen character height as an offset into the scroll
	 * history buffer, as indicated by variable h.  Use -1 to
	 * convert size ss into an index.  Use top as the iterator.
	 * */
	struct JBXVTLine * l = jbxvt_get_saved_lines() + ss - top - 1;
	paint(xc, l, *p);
	p->y += font_height;
	return show_history(xc, line + 1, top - 1, p, font_height, char_size);
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
	int line = show_history(xc, 0, jbxvt_get_scroll() - 1, &p,
		jbxvt_get_font_size().height, chars);
	// Save the position where scroll history ends:
	const int16_t history_end_y = p.y - 1;
	// Do the remainder from the current screen:
	draw_on_screen_lines(xc, &p, line, chars.height);
	if (history_end_y > 0)
		draw_history_line(xc, history_end_y);
	jbxvt_show_selection(xc);
}
