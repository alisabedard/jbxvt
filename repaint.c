/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "repaint.h"

#include "jbxvt.h"
#include "paint.h"
#include "show_selection.h"

#define CSZ jbxvt.scr.chars
#define FSZ jbxvt.X.f.size

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
		for (i = 0, r = *rvec; i < len && rvec[i] == r; ++i)
			;
		// draw
		paint_rval_text(str, r, i, p);
		// advance to next block
		p.x += i * FSZ.width;
		str += i;
		rvec += i;
		len -= i;
	}
}

static int_fast32_t repaint_generic(struct JBDim p, uint_fast16_t len,
	uint8_t * restrict str, uint32_t * rend)
{
	// check inputs:
	if (!str || !len)
		  return p.y + FSZ.height;
	if (rend)
		paint_rvec_text(str, rend + 0, len, p);
	else
		paint_rval_text(str, 0, len, p);
	p.x += len * FSZ.width;
	const uint16_t width = (CSZ.width + 1 - len)
		* FSZ.width;
	xcb_clear_area(jbxvt.X.xcb, false, jbxvt.X.win.vt,
		p.x, p.y, width, FSZ.height);
	return p.y + FSZ.height;
}

static int_fast16_t show_scroll_history(struct JBDim * restrict p)
{
	int_fast16_t line = 0;
	for (int_fast16_t i = jbxvt.scr.offset - 1;
		line <= CSZ.height && i >= 0; ++line, --i) {
		struct JBXVTSavedLine * sl = &jbxvt.scr.sline.data[i];
		p->y = repaint_generic(*p, sl->sl_length,
			sl->text, sl->rend);
	}
	return line;
}

__attribute__((nonnull(1)))
static uint_fast16_t filter_string(uint8_t * restrict buf,
	uint8_t * restrict input)
{
	if (!input)
		return 0;
	uint_fast16_t x;
	for (x = 0; x < CSZ.width; ++x)
		buf[x] = input[x] < ' ' ? ' ' : input[x];
	return x;
}

// Repaint the screen
void repaint(void)
{
	//  First do any 'scrolled off' lines that are visible.
	struct JBDim p = {};
	int_fast32_t line = show_scroll_history(&p);
	// Do the remainder from the current screen:
	for (uint_fast16_t i = 0; line <= CSZ.height; ++line, ++i) {
		// Allocate enough space to process each column
		uint8_t str[CSZ.width];
		p.y = repaint_generic(p, filter_string(str,
			jbxvt.scr.current->text[i]), str,
			jbxvt.scr.current->rend[i]);
	}
	show_selection(0, CSZ.height, 0, CSZ.width);
}

