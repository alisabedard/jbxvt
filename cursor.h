/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_CURSOR_H
#define JBXVT_CURSOR_H
#include <xcb/xcb.h>
void jbxvt_save_cursor(void);
void jbxvt_restore_cursor(xcb_connection_t * xc);
//  Draw the cursor at the current position.
void jbxvt_draw_cursor(xcb_connection_t * xc)
	__attribute__((nonnull));
#endif//!JBXVT_CURSOR_H
