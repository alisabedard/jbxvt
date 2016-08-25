/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_SELECTION_H
#define JBXVT_SELECTION_H

#include "libjb/size.h"
#include <xcb/xcb.h>

//  The current selection unit
enum selunit { SEL_CHAR, SEL_WORD, SEL_LINE };

/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.  */
void check_selection(const int16_t row1, const int16_t row2);

// clear the current selection:
void scr_clear_selection(void);

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(void);

//  respond to a request for our current selection.
void scr_send_selection(const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property);

// start selection using specified unit:
void scr_start_selection(const struct JBDim p, enum selunit unit);

#endif//!JBXVT_SELECTION_H
