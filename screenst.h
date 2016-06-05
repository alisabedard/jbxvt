/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREENST_H
#define SCREENST_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>

/*  Structure describing the current state of the screen.
 */
struct screenst {
	uint8_t **text;		// backup copy of text
	uint32_t **rend;	// rendition styles
	Size margin;		// scroll margins, top and bottom
	xcb_point_t cursor;	// cursor position, row and column
	bool decom:1;		// origin mode flag
	bool wrap:1;		// auto-wrap flag
	bool wrap_next:1;	// wrap before the next printed character
	bool insert:1;		// insert mode flag
};

#endif//!SCREENST_H
