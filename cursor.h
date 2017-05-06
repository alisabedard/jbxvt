/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_CURSOR_H
#define JBXVT_CURSOR_H
#include <xcb/xcb.h>
#include "JBXVTScreen.h"
#include "screen.h"
void jbxvt_blink_cursor(xcb_connection_t * xc);
// Ensure cursor coordinates are valid per screen and decom mode
// Returns new cursor y value
int16_t jbxvt_check_cursor_position(void);
//  Draw the cursor at the current position.
void jbxvt_draw_cursor(xcb_connection_t * xc)
	__attribute__((nonnull));
inline struct JBDim jbxvt_get_cursor(void)
{
	return jbxvt_get_current_screen()->cursor;
}
inline int16_t jbxvt_get_x(void)
{
	return jbxvt_get_cursor().x;
}
inline int16_t jbxvt_get_y(void)
{
	return jbxvt_get_cursor().y;
}
void jbxvt_restore_cursor(xcb_connection_t * xc)
	__attribute__((nonnull));
void jbxvt_save_cursor(void);
void jbxvt_set_cursor_attr(const uint8_t val);
#endif//!JBXVT_CURSOR_H
