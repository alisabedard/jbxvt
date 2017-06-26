/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SCROLL_H
#define JBXVT_SCROLL_H
#include <stdint.h>
#include <xcb/xcb.h>
struct JBXVTLine * jbxvt_get_saved_lines(void);
int16_t jbxvt_get_scroll_size(void);
void jbxvt_clear_scroll_history(void);
void scroll(xcb_connection_t * xc, const uint8_t row1,
	const uint8_t row2, const int16_t count);
#endif//!JBXVT_SCROLL_H
