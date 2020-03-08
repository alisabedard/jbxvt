// Copyright 2017, Jeffrey E. Bedard
#include "size.h"
#include "font.h"
#include "libjb/JBDim.h"
static struct JBDim size_in_pixels, size_in_chars;
void jbxvt_set_pixel_size(const struct JBDim size)
{
    size_in_pixels = size;
    size_in_chars = jbxvt_pixels_to_chars(size);
}
// Return size module's stored pixel size
struct JBDim jbxvt_get_pixel_size(void)
{
    return size_in_pixels;
}
// Return size module's stored size converted to characters
struct JBDim jbxvt_get_char_size(void)
{
    return size_in_chars;
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
