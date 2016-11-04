/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SCREEN_H
#define JBXVT_SCREEN_H
#include <stdbool.h>
#include <xcb/xcb.h>
//  Change between the alternate and the main screens
//  mode_high is true for screen 2
void jbxvt_change_screen(xcb_connection_t * xc, const bool mode_high);
// Set all chars to 'E'
void jbxvt_efill(xcb_connection_t * xc);
// Scroll from top to current bottom margin count lines, moving cursor
void jbxvt_index_from(xcb_connection_t * xc,
	const int8_t count, const int16_t top);
#endif//!JBXVT_SCREEN_H
