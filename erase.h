/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef JBXVT_ERASE_H
#define JBXVT_ERASE_H
#include <stdint.h>
#include <xcb/xcb.h>
enum {
	JBXVT_ERASE_AFTER = 0,
	JBXVT_ERASE_BEFORE = 1,
	JBXVT_ERASE_ALL = 2,
	JBXVT_ERASE_SAVED = 3
};
// Erase the specified portion of a line.
void jbxvt_erase_line(xcb_connection_t * xc, const int8_t mode);
// Erase the specified portion of the screen.
void jbxvt_erase_screen(xcb_connection_t * xc, const int8_t mode);
#endif//!JBXVT_ERASE_H
