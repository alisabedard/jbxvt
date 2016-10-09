#ifndef CHANGE_SELECTION_H
#define CHANGE_SELECTION_H

#include "selend.h"

/*  Repaint the displayed selection to reflect the new value.  ose1 and ose2
 *  are assumed to represent the currently displayed selection endpoints.  */
void jbxvt_change_selection(struct JBDim * restrict ose1,
	struct JBDim * restrict ose2);

#endif//!CHANGE_SELECTION_H
