// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_SIZE_H
#define JBXVT_SIZE_H
#include "libjb/size.h"
// Convert char size/position to pixel size/position
struct JBDim jbxvt_chars_to_pixels(struct JBDim c)
	__attribute__((pure));
/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void jbxvt_fix_coordinates(struct JBDim * restrict rc);
// Return size module's stored size converted to characters
struct JBDim jbxvt_get_char_size(void);
// Return size module's stored pixel size
struct JBDim jbxvt_get_pixel_size(void);
// Convert pixel size/position to char size/position
struct JBDim jbxvt_pixels_to_chars(struct JBDim p)
	__attribute__((pure));
void jbxvt_set_pixel_size(const struct JBDim size);
#endif//!JBXVT_SIZE_H
