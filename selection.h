/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_SELECTION_H
#define JBXVT_SELECTION_H

#include "jbxvt.h"
#include "selst.h"

#include <stdint.h>

enum {
	MAX_WIDTH = 250, // max width of selected lines
	PROP_SIZE = 1024, // chunk size for retrieving the selection property
	SEL_KEY_DEL = 2000 /* time delay in allowing keyboard input to be accepted
			    before a selection arrives. */
};

//  The current selection unit
enum selunit { SEL_CHAR, SEL_WORD, SEL_LINE };

//  respond to a request for our current selection.
void scr_send_selection(const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property);

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(const xcb_time_t time);

/*  Fix the coordinates so that they are within the screen and do not lie within
 *  empty space.  */
void fix_rc(xcb_point_t * restrict rc);

/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.  */
void check_selection(const int16_t row1, const int16_t row2);

//  Convert the selection into a row and column.
void selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct selst * restrict se);

//  Convert a row and column coordinates into a selection endpoint.
void rc_to_selend(const int16_t row, const int16_t col, struct selst * se);

/*  Adjust the selection to a word or line boundary. If the include endpoint is
 *  non NULL then the selection is forced to be large enough to include it.
 */
void adjust_selection(struct selst * restrict include);

// clear the current selection:
void scr_clear_selection(void);

// start selection using specified unit:
void scr_start_selection(const xcb_point_t p, enum selunit unit);

/*  Convert a section of displayed text line into a text string suitable
    for pasting. *lenp is the length of the input string, i1 is index
    of the first character to convert and i2 is the last.  The length
    of the returned string is returned in *lenp; */
uint8_t * convert_line(uint8_t * restrict str,
	uint16_t * restrict lenp, uint8_t i1, uint8_t i2);

#endif//!JBXVT_SELECTION_H
