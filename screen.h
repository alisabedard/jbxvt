/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREEN_H
#define SCREEN_H

#include "handle_sgr.h"
#include "libjb/size.h"

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

// Convert pixel size/position to char size/position
struct JBDim get_c(struct JBDim p)
	__attribute__((pure));

// Convert char size/position to pixel size/position
struct JBDim get_p(struct JBDim c)
	__attribute__((pure));

/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void fix_rc(struct JBDim * restrict rc);

//  Change the rendition style.
void scr_style(const enum RenderFlag style);

//  Change between the alternate and the main screens
//  mode_high is true for screen 2
void scr_change_screen(const bool mode_high);

// Set all chars to 'E'
void scr_efill(void);

// Scroll from top to current bottom margin count lines, moving cursor
void scr_index_from(const int8_t count, const int16_t top);

#endif//!SCREEN_H
