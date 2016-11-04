// Copyright 2016, Jeffrey E. Bedard
#include "size.h"
#include "font.h"
#include "jbxvt.h"
#include "libjb/util.h"
static struct JBDim size_in_pixels;
/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void jbxvt_fix_coordinates(struct JBDim * restrict rc)
{
	struct JBDim c = jbxvt_get_char_size();
	if (!c.w || !c.h)
		return; // prevent segfault on bad window size.
	JB_LIMIT(rc->x, c.w - 1, 0);
	JB_LIMIT(rc->y, c.h - 1, 0);
}
void jbxvt_set_pixel_size(const struct JBDim size)
{
	size_in_pixels = size;
}
// Return size module's stored pixel size
struct JBDim jbxvt_get_pixel_size(void)
{
	return size_in_pixels;
}
// Return size module's stored size converted to characters
struct JBDim jbxvt_get_char_size(void)
{
	return jbxvt_pixels_to_chars(size_in_pixels);
}
// Convert pixel size/position to char size/position
struct JBDim jbxvt_pixels_to_chars(struct JBDim p)
{
	const struct JBDim f = jbxvt_get_font_size();
	p.w /= f.w;
	p.h /= f.h;
	return p;
}
// Convert char size/position to pixel size/position
struct JBDim jbxvt_chars_to_pixels(struct JBDim p)
{
	const struct JBDim f = jbxvt_get_font_size();
	p.w *= f.w;
	p.h *= f.h;
	return p;
}
