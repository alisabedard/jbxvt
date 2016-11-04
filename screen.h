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
void jbxvt_fix_coordinates(struct JBDim * restrict rc);
// Get default colormap for the screen
xcb_colormap_t jbxvt_get_colormap(xcb_connection_t * xc);
// Get the root window of the screen
xcb_window_t jbxvt_get_root_window(xcb_connection_t * xc);
//  Change between the alternate and the main screens
//  mode_high is true for screen 2
void jbxvt_change_screen(xcb_connection_t * xc, const bool mode_high);
// Set all chars to 'E'
void jbxvt_efill(xcb_connection_t * xc);
// Scroll from top to current bottom margin count lines, moving cursor
void jbxvt_index_from(xcb_connection_t * xc,
	const int8_t count, const int16_t top);
#endif//!JBXVT_SCREEN_H
