/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SCROLL_H
#define JBXVT_SCROLL_H
#include <xcb/xcb.h>
struct JBXVTLine * jbxvt_get_saved_lines(void);
int jbxvt_get_scroll_size(void);
void jbxvt_clear_scroll_history(void);
void scroll(xcb_connection_t * xc, const int row1,
	const int row2, const int count);
#endif//!JBXVT_SCROLL_H
