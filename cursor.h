/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_CURSOR_H
#define JBXVT_CURSOR_H
#include <xcb/xcb.h>
#include <xcb/xproto.h>
void jbxvt_blink_cursor(xcb_connection_t * xc);
// Ensure cursor coordinates are valid per screen and decom mode
// Returns new cursor y value
int16_t jbxvt_check_cursor_position(void);
struct JBDim jbxvt_get_cursor(void);
int16_t jbxvt_get_x(void);
int16_t jbxvt_get_y(void);
void jbxvt_save_cursor(void);
void jbxvt_set_cursor_attr(const uint8_t val);
void jbxvt_restore_cursor(xcb_connection_t * xc);
//  Draw the cursor at the current position.
void jbxvt_draw_cursor(xcb_connection_t * xc)
	__attribute__((nonnull));
#endif//!JBXVT_CURSOR_H
