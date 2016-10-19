/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SCREEN_H
#define JBXVT_SCREEN_H
#include "libjb/size.h"
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
// Convert pixel size/position to char size/position
struct JBDim jbxvt_get_char_size(struct JBDim p)
	__attribute__((pure));
// Convert char size/position to pixel size/position
struct JBDim jbxvt_get_pixel_size(struct JBDim c)
	__attribute__((pure));
/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void fix_rc(struct JBDim * restrict rc);
//  Change the rendition style.
void jbxvt_style(const uint32_t style);
//  Change between the alternate and the main screens
//  mode_high is true for screen 2
void jbxvt_change_screen(const bool mode_high);
// Set all chars to 'E'
void jbxvt_efill(void);
// Scroll from top to current bottom margin count lines, moving cursor
void jbxvt_index_from(const int8_t count, const int16_t top);
#endif//!JBXVT_SCREEN_H
