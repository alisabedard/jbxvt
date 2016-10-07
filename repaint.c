/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "repaint.h"

#include "jbxvt.h"
#include "paint.h"
#include "show_selection.h"

/* Display the string using the rendition vector
   at the screen coordinates.  */
static void paint_rvec_text(uint8_t * str, uint32_t * rvec,
	int16_t len, struct JBDim p)
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
#define FSZ jbxvt.X.f.size
		p.x += i * FSZ.width;
		str += i;
		rvec += i;
		len -= i;
	}
}

static int16_t repaint_generic(struct JBDim p, const int16_t m,
	const int16_t c1, const int16_t c2, uint8_t * restrict str,
	uint32_t * rend)
{
	// check inputs:
	if (!str || !m)
		  return p.y + FSZ.height;
	if (rend && c1 <= jbxvt.scr.chars.width)
		paint_rvec_text(str, rend + c1, m, p);
	else
		paint_rval_text(str, 0, m, p);
	p.x += m * FSZ.width;
	const uint16_t width = (c2 - c1 + 1 - m) * FSZ.width;
	xcb_clear_area(jbxvt.X.xcb, false, jbxvt.X.win.vt,
		p.x, p.y, width, FSZ.height);
	return p.y + FSZ.height;
#undef FSZ
}

static int_fast16_t show_scroll_history(struct JBDim * restrict p)
{
	int_fast16_t line = 0;
	for (int_fast16_t i = jbxvt.scr.offset - 1;
		line <= jbxvt.scr.chars.height && i >= 0; ++line, --i) {
		struct JBXVTSavedLine * sl = &jbxvt.scr.sline.data[i];
		p->y = repaint_generic(*p, sl->sl_length,
			0, jbxvt.scr.chars.width, sl->text, sl->rend);
	}
	return line;
}

// Repaint the screen
void repaint(void)
{
	struct JBDim p = {};
	int_fast16_t y = show_scroll_history(&p);
	uint8_t str[jbxvt.scr.chars.width];
	for (uint_fast16_t i = 0; y <= jbxvt.scr.chars.height; ++y, ++i) {
		uint8_t * s = jbxvt.scr.current->text[i];
		int_fast16_t x = 0;
		for (x = 0; s[x]; ++x)
			str[x] = s[x] < ' ' ? ' ' : s[x];
		p.y = x > 0 ? repaint_generic(p, x, 0, jbxvt.scr.chars.width,
			str, jbxvt.scr.current->rend[i])
			: jbxvt.X.f.size.h + p.y;
	}
	show_selection(0, jbxvt.scr.chars.height, 0, jbxvt.scr.chars.width);
	xcb_flush(jbxvt.X.xcb);
}

