/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREEN_H
#define SCREEN_H

#include "handle_sgr.h"

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void fix_rc(xcb_point_t * restrict rc);

//  Change the rendition style.
void scr_style(const enum RenderFlag style);

//  Change between the alternate and the main screens
//  mode_high is true for screen 2
void scr_change_screen(const bool mode_high);

// Set all chars to 'E'
void scr_efill(void);

// Scroll from top to current bottom margin count lines, moving cursor
void scr_index_from(const int8_t count, const int16_t top);

/*  Move the display so that line represented by scrollbar value y is at the top
 *  of the screen.  */
void scr_move_to(int16_t y);

//  Send the name of the current display to the command.
void scr_report_display(void);

#endif//!SCREEN_H
