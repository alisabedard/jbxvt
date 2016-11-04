// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_SIZE_H
#define JBXVT_SIZE_H
#include "libjb/size.h"
/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void jbxvt_fix_coordinates(struct JBDim * restrict rc);
// Convert pixel size/position to char size/position
struct JBDim jbxvt_get_char_size(struct JBDim p)
	__attribute__((pure));
// Convert char size/position to pixel size/position
struct JBDim jbxvt_get_pixel_size(struct JBDim c)
	__attribute__((pure));
#endif//!JBXVT_SIZE_H
