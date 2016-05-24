/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_CURSOR_H
#define JBXVT_CURSOR_H

enum CursorOp {
	CURSOR_DRAW,
	CURSOR_SAVE,
	CURSOR_RESTORE,
	CURSOR_REPORT,
	CURSOR_FOCUS_IN,
	CURSOR_FOCUS_OUT,
	CURSOR_ENTRY_IN,
	CURSOR_ENTRY_OUT
};

//  Draw the cursor at the current position.
void cursor(const enum CursorOp op);

#endif//!JBXVT_CURSOR_H
