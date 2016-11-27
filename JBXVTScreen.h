/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef SCREENST_H
#define SCREENST_H
#include <stdbool.h>
#include "config.h"
#include "libjb/JBDim.h"
/*  Structure describing the current state of the screen.  */
struct JBXVTScreen {
	uint8_t *text[JBXVT_MAX_ROWS];	// text
	uint32_t *rend[JBXVT_MAX_ROWS];	// styles
	bool wrap[JBXVT_MAX_ROWS];	// wrap flags
	bool dwl[JBXVT_MAX_ROWS];	// double-width line flags
	struct JBDim margin;	// scroll margins, top and bottom
	struct JBDim cursor;	// cursor position, row and column
	bool wrap_next:1;	// wrap before the next printed character
	bool decpm:1;		// privacy message
	bool cursor_visible:1;
};
#endif//!SCREENST_H
