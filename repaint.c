/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "repaint.h"
#include "font.h"
#include "paint.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "show_selection.h"
#include "size.h"
#include "window.h"
static uint_fast16_t get_render_length(const rstyle_t * rvec, const uint16_t len)
{
	uint_fast16_t i = 0;
	while (i < len && rvec[i] == rvec[0])
		++i;
	return i;
}
// Display the string using the render vector at the screen coordinates.
static void paint_rvec_text(xcb_connection_t * xc,
	uint8_t * str, const rstyle_t * rvec,
	uint16_t len, struct JBDim p, const bool dwl)
{
	const uint8_t fw = jbxvt_get_font_size().width;
	for (uint_fast16_t i; len; len -= i, str += i, rvec += i, p.x += i * fw)
		jbxvt_paint(xc, str, *rvec, i = get_render_length(rvec, len), p,
			dwl);
}
__attribute__((nonnull))
static int_fast16_t show_scroll_history(xcb_connection_t * xc,
	struct JBDim * restrict p, const int_fast16_t line,
	const int_fast16_t i, const struct JBDim font_size)
{
	const struct JBDim chars = jbxvt_get_char_size();
	if (line > chars.h || i < 0)
		return line;
	struct JBXVTSavedLine * sl = &jbxvt_get_saved_lines()[i];
	paint_rvec_text(xc, sl->text, sl->rend, sl->size, *p, sl->dwl);
	{ // x, w scope
		const int16_t x = sl->size * font_size.width;
		const uint16_t w = chars.width * font_size.width - x;
		xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), x, p->y,
			w, font_size.height);
	}
	p->y += font_size.height;
	return show_scroll_history(xc, p, line + 1, i - 1, font_size);
}
static uint8_t * filter(uint8_t * restrict t, register int_fast16_t i)
{
	while (--i >= 0)
		if (t[i] < ' ')
			t[i] = ' ';
	return t;
}
// Repaint the screen
void jbxvt_repaint(xcb_connection_t * xc)
{
	//  First do any 'scrolled off' lines that are visible.
	struct JBDim p = {};
	const struct JBDim chars = jbxvt_get_char_size(),
	      f = jbxvt_get_font_size();
	int_fast32_t line = show_scroll_history(xc,
		&p, 0, jbxvt_get_scroll() - 1, f);
	// Do the remainder from the current screen:
	struct JBXVTScreen * s = jbxvt_get_current_screen();
	for (uint_fast16_t i = 0; line < chars.height;
		++line, ++i, p.y += f.height)
		paint_rvec_text(xc, filter(s->text[i], chars.width),
			s->rend[i], chars.width, p, s->dwl[i]);
	jbxvt_show_selection(xc);
}
