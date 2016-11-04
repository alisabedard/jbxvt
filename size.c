// Copyright 2016, Jeffrey E. Bedard
#include "size.h"
#include "font.h"
#include "jbxvt.h"
/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void jbxvt_fix_coordinates(struct JBDim * restrict rc)
{
	if(!jbxvt.scr.chars.h || !jbxvt.scr.chars.w)
		  return; // prevent segfault on bad window size.
	JB_LIMIT(rc->x, jbxvt.scr.chars.w - 1, 0);
	JB_LIMIT(rc->y, jbxvt.scr.chars.h - 1, 0);
}
struct JBDim jbxvt_get_char_size(struct JBDim p)
{
	const struct JBDim f = jbxvt_get_font_size();
	p.w /= f.w;
	p.h /= f.h;
	return p;
}
struct JBDim jbxvt_get_pixel_size(struct JBDim p)
{
	const struct JBDim f = jbxvt_get_font_size();
	p.w *= f.w;
	p.h *= f.h;
	return p;
}
