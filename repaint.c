/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "repaint.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "paint.h"
#include "show_selection.h"

#include <stdlib.h>
#include <string.h>

/* Display the string using the rendition vector
   at the screen coordinates.  */
static void paint_rvec_text(uint8_t * str, uint32_t * rvec,
	int16_t len, xcb_point_t p)
{
	if (!rvec || !str)
		  return;
	while (len > 0) {
		uint32_t r;
		int_fast16_t i;
		/* Find the length for which
		   the current rend val applies.  */
		for (i = 0, r = *rvec; i < len
			&& rvec[i] == r; ++i)
			;
		// draw
		paint_rval_text(str, r, i, p);
		// advance to next block
		p.x += i * jbxvt.X.f.size.width;
		str += i;
		rvec += i;
		len -= i;
	}
}

static int_fast32_t repaint_generic(xcb_point_t p,
	int_fast16_t m, const int_fast32_t c1,
	const int_fast32_t c2, uint8_t * restrict str,
	uint32_t * rend)
{
	const Size f = jbxvt.X.f.size;
	// check inputs:
	if (!str || !m)
		  return p.y + f.height;
	if (rend && c1 <= jbxvt.scr.chars.width)
		paint_rvec_text(str, rend + c1, m, p);
	else
		paint_rval_text(str, 0, m, p);
	p.x += m * f.width;
	const uint16_t width = (c2 - c1 + 1 - m) * f.width;
	xcb_clear_area(jbxvt.X.xcb, false, jbxvt.X.win.vt,
		p.x, p.y, width, f.height);
	return p.y + f.height;
}

static int_fast16_t show_scroll_history(xcb_rectangle_t r,
	xcb_point_t * restrict p)
{
	int_fast16_t line = r.y;
	for (int_fast16_t i = jbxvt.scr.offset - r.y - 1;
		line <= r.height && i >= 0; ++line, --i) {
		SLine * sl = jbxvt.scr.sline.data[i];
		if (!sl) // no scroll history
			break;
		p->y = repaint_generic(*p, sl->sl_length,
			r.x, r.width, sl->sl_text, sl->sl_rend);
	}
	return line;
}

// Repaint the screen
void repaint(void)
{
	const xcb_rectangle_t r = {.width = jbxvt.scr.chars.width,
		.height = jbxvt.scr.chars.height};
	xcb_point_t p = { .x = MARGIN + r.x
		* jbxvt.X.f.size.width,
		.y = MARGIN + r.y * jbxvt.X.f.size.height};
	/* Allocate enough space to process each column, plus
	 * wrap byte. */
	uint8_t str[jbxvt.scr.chars.width + 1];
	//  First do any 'scrolled off' lines that are visible.
	int_fast32_t line = show_scroll_history(r, &p);

	// Do the remainder from the current screen:
	int_fast32_t i = jbxvt.scr.offset > r.y ? 0
		: r.y - jbxvt.scr.offset;

	for (; line <= r.height; ++line, ++i) {
		uint8_t * s = jbxvt.scr.current->text[i];
		register int_fast16_t x;
		for (x = r.x; s && x <= r.width
			&& x < jbxvt.scr.chars.width; ++x)
			str[x - r.x] = s[x] < ' ' ? ' ' : s[x];
		const uint16_t m = x - r.x;
		p.y = repaint_generic(p, m, r.x, r.width, str,
			jbxvt.scr.current->rend[i]);
	}
	show_selection(r.y,r.height,r.x,r.width);
}

