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
/* Display the string using the rendition vector
   at the screen coordinates.  */
static void paint_rvec_text(xcb_connection_t * xc,
	uint8_t * str, uint32_t * rvec,
	int16_t len, struct JBDim p, const bool dwl)
{
	if (!rvec || !str)
		  return;
	while (len > 0) {
		uint32_t r;
		int_fast16_t i;
		/* Find the length for which
		   the current rend val applies.  */
		for (i = 0, r = *rvec; i < len && rvec[i] == r; ++i)
			;
		// draw
		jbxvt_paint(xc, str, r, i, p, dwl);
		// advance to next block
		p.x += i * jbxvt_get_font_size().width;
		str += i;
		rvec += i;
		len -= i;
	}
}
static int_fast32_t repaint_generic(xcb_connection_t * xc,
	struct JBDim p, uint_fast16_t len,
	uint8_t * restrict str, uint32_t * rend, const bool dwl)
{
	// check inputs:
	if (!str || !len)
		return p.y + jbxvt_get_font_size().height;
	if (rend)
		paint_rvec_text(xc, str, rend + 0, len, p, dwl);
	else
		jbxvt_paint(xc, str, 0, len, p, dwl);
	p.x += len * jbxvt_get_font_size().width;
	const uint16_t width = (jbxvt_get_char_size().width + 1 - len)
		* jbxvt_get_font_size().width;
	xcb_clear_area(xc, false, jbxvt_get_vt_window(xc), p.x, p.y,
		width, jbxvt_get_font_size().height);
	return p.y + jbxvt_get_font_size().height;
}
__attribute__((nonnull(1)))
static int_fast16_t show_scroll_history(xcb_connection_t * xc,
	struct JBDim * restrict p, const int_fast16_t line,
	const int_fast16_t i)
{
	if (line > jbxvt_get_char_size().h || i < 0)
		return line;
	struct JBXVTSavedLine * sl = &jbxvt_get_saved_lines()[i];
	p->y = repaint_generic(xc, *p, sl->size, sl->text,
		sl->rend, sl->dwl);
	return show_scroll_history(xc, p, line + 1, i - 1);
}
__attribute__((nonnull(1)))
static uint_fast16_t filter_string(uint8_t * restrict buf,
	uint8_t * restrict input)
{
	if (!input)
		return 0;
	uint_fast16_t x;
	for (x = 0; x < jbxvt_get_char_size().width; ++x)
		buf[x] = input[x] < ' ' ? ' ' : input[x];
	return x;
}
// Repaint the screen
void jbxvt_repaint(xcb_connection_t * xc)
{
	//  First do any 'scrolled off' lines that are visible.
	struct JBDim p = {};
	int_fast32_t line = show_scroll_history(xc,
		&p, 0, jbxvt_get_scroll() - 1);
	// Do the remainder from the current screen:
	for (uint_fast16_t i = 0; line < jbxvt_get_char_size().height;
		++line, ++i) {
		// Allocate enough space to process each column
		uint8_t str[jbxvt_get_char_size().width];
		struct JBXVTScreen * s = jbxvt_get_screen();
		p.y = repaint_generic(xc, p, filter_string(str, s->text[i]),
			str, s->rend[i], s->dwl[i]);
	}
	jbxvt_show_selection(xc);
}
