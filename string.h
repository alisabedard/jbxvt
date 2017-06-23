/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef JBXVT_STRING_H
#define JBXVT_STRING_H
#include <xcb/xcb.h>
/*  Display the string at the current position.
    new_line_count is the number of new lines in the string.  */
void jbxvt_string(xcb_connection_t * xc, uint8_t * restrict str, int len,
	const int new_line_count);
#endif//!JBXVT_STRING_H
