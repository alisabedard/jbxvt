/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREENST_H
#define SCREENST_H

#include "libjb/size.h"

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>


/*  Structure describing the current state of the screen.  */
struct JBXVTScreen {
	uint8_t **text;		// backup copy of text
	uint32_t **rend;	// rendition styles
	bool * wrap;		// wrap flags
	struct JBDim margin;	// scroll margins, top and bottom
	struct JBDim cursor;	// cursor position, row and column
	bool wrap_next:1;	// wrap before the next printed character
	bool decpm:1;		// privacy message
	bool cursor_visible:1;
};

#endif//!SCREENST_H