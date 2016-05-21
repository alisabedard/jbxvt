/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_CURSOR_H
#define JBXVT_CURSOR_H

#include <stdbool.h>
#include <stdint.h>

//  Draw the cursor at the current position.
void cursor(void);

enum ScreenFocusFlags {
	SCR_FOCUS_IN = 1,
	SCR_FOCUS_ENTRY = 2,
	SCR_FOCUS_FOCUS = 4
};

/*  Indicate a change of keyboard focus.  Type is 1 if focusing in,
    2 for entry events, and 4 for focus events.  */
void scr_focus(const uint8_t flags);

#endif//!JBXVT_CURSOR_H
