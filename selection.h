/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_SELECTION_H
#define JBXVT_SELECTION_H

#include "jbxvt.h"

//  The current selection unit
enum selunit { SEL_CHAR, SEL_WORD, SEL_LINE };

//  respond to a request for our current selection.
void scr_send_selection(const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property);

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(void);

/*  Fix the coordinates so that they are within the screen and do not lie within
 *  empty space.  */
void fix_rc(xcb_point_t * restrict rc);

/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.  */
void check_selection(const int16_t row1, const int16_t row2);

//  Convert the selection into a row and column.
void selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	SelEnd * restrict se);

//  Convert a row and column coordinates into a selection endpoint.
void rc_to_selend(const int16_t row, const int16_t col, SelEnd * se);

/*  Adjust the selection to a word or line boundary. If the include endpoint is
 *  non NULL then the selection is forced to be large enough to include it.
 */
void adjust_selection(SelEnd * restrict include);

// clear the current selection:
void scr_clear_selection(void);

// start selection using specified unit:
void scr_start_selection(const xcb_point_t p, enum selunit unit);

#endif//!JBXVT_SELECTION_H
