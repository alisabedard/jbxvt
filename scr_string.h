/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef JBXVT_SCR_STRING_H
#define JBXVT_SCR_STRING_H
#include <stdint.h>
#include <xcb/xcb.h>
/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void jbxvt_string(xcb_connection_t * xc,
	uint8_t * restrict str, uint8_t len, int8_t nlcount);
#endif//!JBXVT_SCR_STRING_H
