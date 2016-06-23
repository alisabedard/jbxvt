/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREENST_H
#define SCREENST_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>

enum CharacterSet {
	CHARSET_GB, CHARSET_ASCII, CHARSET_SG0, CHARSET_SG1, CHARSET_SG2
};

/*  Structure describing the current state of the screen.
 */
typedef struct screenst {
	uint8_t **text;		// backup copy of text
	uint32_t **rend;	// rendition styles
	Size margin;		// scroll margins, top and bottom
	xcb_point_t cursor;	// cursor position, row and column
	uint8_t charset[2];	// graphics mode char set
	bool decom:1;		// origin mode flag
	bool decawm:1;		// DECAWM auto-wrap flag
	bool wrap_next:1;	// wrap before the next printed character
	bool insert:1;		// insert mode flag
	bool decanm:1;		// DECANM -- ANSI/VT52
	bool dectcem:1;		// DECTCEM -- hide cursor
	bool ptr_xy:1;		// send x y on button press/release
	bool ptr_cell:1;	// cell motion mouse tracking
	uint8_t charsel:1;	// charset index
} VTScreen;

#endif//!SCREENST_H
