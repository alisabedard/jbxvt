/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef JBXVT_SCR_STRING_H
#define JBXVT_SCR_STRING_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
/*  Display the string at the current position.
    nlcount is the number of new lines in the string.  */
void jbxvt_string(xcb_connection_t * xc,
	uint8_t * restrict str, uint8_t len, int8_t nlcount);
//  Tab to the next tab stop.
void jbxvt_tab(xcb_connection_t * xc);
// Do v tabs
void jbxvt_cht(xcb_connection_t * xc, int16_t v);
// Set tab stops:
// -1 clears all, -2 sets default
void jbxvt_set_tab(int16_t i, const bool value);
#endif//!JBXVT_SCR_STRING_H
