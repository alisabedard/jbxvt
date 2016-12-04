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
static uint16_t get_render_length(rstyle_t * rvec, const uint16_t len)
{
	uint_fast16_t i = 0;
	for(;i < len && rvec[i] == rvec[0]; ++i)
		;
	return i;
}
/* Display the string using the rendition vector
   at the screen coordinates.  */
static void paint_rvec_text(xcb_connection_t * xc,
	uint8_t * str, rstyle_t * rvec,
	uint16_t len, struct JBDim p, const bool dwl)
{
	/* Save the retrieved font width to avoid function call overhead.  */
	static uint8_t fw;
	if (!fw)
		fw = jbxvt_get_font_size().width;
	while (len) {
		const uint16_t rlen = get_render_length(rvec, len);
		jbxvt_paint(xc, str, rvec[0], rlen, p, dwl);
		// advance to next block:
		p.x += rlen * fw;
		str += rlen;
		rvec += rlen;
		len -= rlen;
	}
}
__attribute__((nonnull))
static int_fast16_t show_scroll_history(xcb_connection_t * xc,
	struct JBDim * restrict p, const int_fast16_t line,
	const int_fast16_t i)
{
	const struct JBDim chars = jbxvt_get_char_size();
	if (line > chars.h || i < 0)
		return line;
	struct JBXVTSavedLine * sl = &jbxvt_get_saved_lines()[i];
	paint_rvec_text(xc, sl->text, sl->rend, chars.width, *p, sl->dwl);
	const struct JBDim f = jbxvt_get_font_size();
	const int16_t x = sl->size * f.width;
	const uint16_t w = chars.width * f.width - x;
	xcb_clear_area(xc, false, jbxvt_get_vt_window(xc), x, p->y, w,
		f.height);
	p->y += f.height;
	return show_scroll_history(xc, p, line + 1, i - 1);
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
	int_fast32_t line = show_scroll_history(xc,
		&p, 0, jbxvt_get_scroll() - 1);
	const struct JBDim chars = jbxvt_get_char_size(),
	      f = jbxvt_get_font_size();
	// Do the remainder from the current screen:
	struct JBXVTScreen * s = jbxvt_get_current_screen();
	for (uint_fast16_t i = 0; line < chars.height;
		++line, ++i, p.y += f.height)
		paint_rvec_text(xc, filter(s->text[i], chars.width),
			s->rend[i], chars.width, p, s->dwl[i]);
	jbxvt_show_selection(xc);
}
