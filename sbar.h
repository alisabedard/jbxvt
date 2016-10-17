/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef SBAR_H
#define SBAR_H
#include <stdint.h>
// Draw the scrollbar.
void jbxvt_draw_scrollbar(void);
//  Change the value of the scrolled screen offset and repaint the screen
void jbxvt_set_scroll(int16_t n);
// Scroll to the specified y position (in pixels)
void jbxvt_scroll_to(const int16_t y);
// Clear the scroll history
void jbxvt_clear_saved_lines();
// Show or hide the scroll bar window
void jbxvt_toggle_scrollbar(void);
#endif//!SBAR_H
