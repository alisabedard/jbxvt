#ifndef JBXVT_SELEND_H
#define JBXVT_SELEND_H

#include "libjb/size.h"
#include <stdbool.h>

// selection endpoint types:
enum JBXVTSelType {
	JBXVT_SEL_NONE = false,
	JBXVT_SEL_ON_SCREEN = true,
};

/*  Adjust the selection to a word or line boundary.
    If the include endpoint is non NULL then the selection
    is forced to be large enough to include it.  */
void jbxvt_adjust_selection(struct JBDim * restrict include);

//  Convert a row and column coordinates into a selection endpoint.
void jbxvt_rc_to_selend(const int16_t row, const int16_t col,
	struct JBDim * se);

/*  Compare the two selections and return negtive,
    0 or positive depending on whether se2 is after,
    equal to or before se1.  */
int8_t jbxvt_selcmp(struct JBDim * restrict se1,
	struct JBDim * restrict se2);

//  Convert the selection into a row and column.
void jbxvt_selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct JBDim * restrict se);

#endif//!JBXVT_SELEND_H
