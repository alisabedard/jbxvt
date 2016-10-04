#ifndef CHANGE_SELECTION_H
#define CHANGE_SELECTION_H

#include "selend.h"

/*  Repaint the displayed selection to reflect the new value.  ose1 and ose2
 *  are assumed to represent the currently displayed selection endpoints.  */
void change_selection(struct JBDim * restrict ose1,
	struct JBDim * restrict ose2);

/*  Compare the two selections and return negtive, 0 or positive depending on
 *  whether se2 is after, equal to or before se1.  */
int8_t selcmp(struct JBDim * restrict se1,
	struct JBDim * restrict se2);

#endif//!CHANGE_SELECTION_H
