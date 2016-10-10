/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "repaint.h"

#include "jbxvt.h"
#include "paint.h"
#include "show_selection.h"

#include <string.h>

/* Display the string using the rendition vector
   at the screen coordinates.  */
static void paint_rvec_text(uint8_t * str, uint32_t * rvec,
	int16_t len, struct JBDim p)
{
	if (!len)
		return;
	int_fast16_t i = 0;
	while (i < len && rvec[i] == rvec[0])
		++i;
	paint_rstyle_text(str, rvec[0], i, p);
	p.x += i * jbxvt.X.f.size.width;
	paint_rvec_text(str + i, rvec + i, len - i, p);
}

__attribute__((nonnull(4)))
static int_fast32_t repaint_generic(struct JBDim p, uint_fast16_t len,
	uint8_t * restrict str, uint32_t * rend)
{
	// check inputs:
	if (!str || !len)
		  return p.y + jbxvt.X.f.size.height;
	paint_rvec_text(str, rend + 0, len, p);
	p.x += len * jbxvt.X.f.size.width;
	xcb_clear_area(jbxvt.X.xcb, false, jbxvt.X.win.vt,
		p.x, p.y, jbxvt.scr.pixels.width,
		jbxvt.X.f.size.height);
	return p.y + jbxvt.X.f.size.height;
}

__attribute__((nonnull(1)))
static int_fast16_t show_scroll_history(struct JBDim * restrict p,
	const int_fast16_t line, const int_fast16_t i)
{
	if (line > jbxvt.scr.chars.h || i < 0)
		return line;
	struct JBXVTSavedLine * sl = &jbxvt.scr.sline.data[i];
	p->y = repaint_generic(*p, sl->sl_length, sl->text, sl->rend);
	return show_scroll_history(p, line + 1, i - 1);
}

// Repaint the screen
void repaint(void)
{
	//  First do any 'scrolled off' lines that are visible.
	struct JBDim p = {};
	int_fast32_t line = show_scroll_history(&p, 0, jbxvt.scr.offset - 1);
	// Do the remainder from the current screen:
	for (uint_fast16_t i = 0; line <= jbxvt.scr.chars.height;
		++line, ++i)
		// Allocate enough space to process each column
		p.y = repaint_generic(p,
			strlen((const char *)jbxvt.scr.current->text[i]),
			jbxvt.scr.current->text[i],
			jbxvt.scr.current->rend[i]);
	jbxvt_show_selection();
}

