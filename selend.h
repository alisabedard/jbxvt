#ifndef JBXVT_SELEND_H
#define JBXVT_SELEND_H

#include <stdint.h>
#include <stdbool.h>

//  selection endpoint types.
typedef enum {
	NOSEL,
	SCREENSEL,
	SAVEDSEL
} SelType;

//  structure describing a selection endpoint.
struct JBXVTSelEnd {
	SelType type;
	uint16_t index;	// index into the sline or screen array
	uint8_t col;	// column of the character
};

/*  Adjust the selection to a word or line boundary.
    If the include endpoint is non NULL then the selection
    is forced to be large enough to include it.  */
void adjust_selection(struct JBXVTSelEnd * restrict include);

// Make i positive, return true if it was already positive
bool ipos(int16_t * i);

//  Convert a row and column coordinates into a selection endpoint.
void rc_to_selend(const int16_t row, const int16_t col,
	struct JBXVTSelEnd * se);

/*  Compare the two selections and return negtive,
    0 or positive depending on whether se2 is after,
    equal to or before se1.  */
int8_t selcmp(struct JBXVTSelEnd * restrict se1,
	struct JBXVTSelEnd * restrict se2);

//  Convert the selection into a row and column.
void selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct JBXVTSelEnd * restrict se);

#endif//!JBXVT_SELEND_H
