/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef SCREENST_H
#define SCREENST_H
#include "JBXVTLine.h"
#include "libjb/JBDim.h"
/*  Structure describing the current state of the screen.  */
struct JBXVTScreen {
	struct JBXVTLine line[JBXVT_MAX_ROWS];
	struct JBDim margin;	// scroll margins, top and bottom
	struct JBDim cursor;	// cursor position, row and column
	bool wrap_next:1;	// wrap before the next printed character
	bool decpm:1;		// privacy message
	bool cursor_visible:1;
};
#endif//!SCREENST_H
