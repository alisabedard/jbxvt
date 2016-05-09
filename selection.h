#ifndef JBXVT_SELECTION_H
#define JBXVT_SELECTION_H

#include "selst.h"

#include <stdint.h>
#include <X11/X.h>

enum {
	MAX_WIDTH = 250, // max width of selected lines
	PROP_SIZE = 1024, // chunk size for retrieving the selection property
	SEL_KEY_DEL = 2000 /* time delay in allowing keyboard input to be accepted
			    before a selection arrives. */
};

/*  The current selection unit
 */
enum selunit {
	CHAR,
	WORD,
	LINE
};

/*  respond to a request for our current selection.
 */
void scr_send_selection(const int time __attribute__((unused)),
	const int requestor, const int target, const int property);

/*  Make the selection currently delimited by the selection end markers.
 */
void scr_make_selection(const Time time);

/*  Fix the coordinates so that they are within the screen and do not lie within
 *  empty space.
 */
void fix_rc(int16_t * restrict rowp, int16_t * restrict colp);

/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.
 */
void check_selection(const int16_t row1, const int16_t row2);

/*  Paint any part of the selection that is between rows row1 and row2 inclusive
 *  and between cols col1 and col2 inclusive.
 */
void show_selection(int16_t row1, int16_t row2, int16_t col1, int16_t col2);

/*  Convert the selection into a row and column.
 */
void selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct selst * restrict se);

/*  Convert a row and column coordinates into a selection endpoint.
 */
void rc_to_selend(const int16_t row, const int16_t col, struct selst * se);

/*  Adjust the selection to a word or line boundary. If the include endpoint is
 *  non NULL then the selection is forced to be large enough to include it.
 */
void adjust_selection(struct selst * restrict include);

// clear the current selection:
void scr_clear_selection(void);

// start selection using specified unit:
void scr_start_selection(int x, int y, enum selunit unit);

/*  Convert a section of displayed text line into a text string suitable for pasting.
 *  *lenp is the length of the input string, i1 is index of the first character to
 *  convert and i2 is the last.  The length of the returned string is returned
 *  in *lenp;
 */
unsigned char * convert_line(unsigned char * restrict str,
	int * restrict lenp, int i1, int i2);

#endif//!JBXVT_SELECTION_H
