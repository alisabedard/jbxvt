/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_SELCMP_H
#define JBXVT_SELCMP_H

#include "selst.h"
#include <stdint.h>

/*  Compare the two selections and return -1, 0 or 1 depending on
 *  whether se2 is after, equal to or before se1.  */
int8_t selcmp(struct selst * se1, struct selst * se2);

#endif//!JBXVT_SELCMP_H
