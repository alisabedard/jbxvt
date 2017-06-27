/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SBAR_H
#define JBXVT_SBAR_H
#include <stdbool.h>
#include <xcb/xcb.h>
// Draw the scrollbar.
void jbxvt_draw_scrollbar(xcb_connection_t * xc);
// Get the current scroll position.
int16_t jbxvt_get_scroll(void);
// Get the scrollbar window id
xcb_window_t jbxvt_get_scrollbar(xcb_connection_t * c)
	__attribute__((nonnull));
bool jbxvt_get_scrollbar_visible(void);
//  Change the value of the scrolled screen offset and repaint the screen
void jbxvt_set_scroll(xcb_connection_t * xc, const int16_t new_offset);
// Scroll to the specified y position (in pixels)
void jbxvt_scroll_to(xcb_connection_t * xc, const int16_t y);
// Clear the scroll history
void jbxvt_clear_saved_lines(xcb_connection_t * xc);
// Show or hide the scroll bar window
void jbxvt_toggle_scrollbar(xcb_connection_t * xc);
#endif//!JBXVT_SBAR_H
