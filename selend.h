// Copyright 2017, Jeffrey E. Bedard <jefbed@gmail.com>
#ifndef JBXVT_SELEND_H
#define JBXVT_SELEND_H
#include <stdint.h>
struct JBDim;
/*  Adjust the selection to a word or line boundary.
    If the include endpoint is non NULL then the selection
    is forced to be large enough to include it.  */
void jbxvt_adjust_selection(struct JBDim * restrict include);
// Convert the selection end point into a dimension structure
struct JBDim jbxvt_get_selend_position(struct JBDim * restrict se);
// Make sure selection end point 0 comes before end point 1
struct JBDim * jbxvt_order_selection_ends(struct JBDim * e);
//  Convert a row and column coordinates into a selection endpoint.
void jbxvt_rc_to_selend(const int16_t row, const int16_t col,
    struct JBDim * se);
/*  Compare the two selections and return negtive,
    0 or positive depending on whether se2 is after,
    equal to or before se1.  */
int8_t jbxvt_selcmp(struct JBDim * restrict se1,
    struct JBDim * restrict se2);
#endif//!JBXVT_SELEND_H
