/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selcmp.h"

/*  Compare the two selections and return -1, 0 or 1 depending on
 *  whether se2 is after, equal to or before se1.
 */
int8_t selcmp(struct selst * se1, struct selst * se2)
{
	if (se1->se_type == SAVEDSEL && se2->se_type == SAVEDSEL) {
		if (se1->se_index > se2->se_index)
			return(-1);
		if (se1->se_index < se2->se_index)
			return(1);
		if (se1->se_col < se2->se_col)
			return(-1);
		if (se2->se_col < se1->se_col)
			return(1);
		return(0);
	}
	if (se1->se_type == SCREENSEL && se2->se_type == SCREENSEL) {
		if (se1->se_index < se2->se_index)
			return(-1);
		if (se1->se_index > se2->se_index)
			return(1);
		if (se1->se_col < se2->se_col)
			return(-1);
		if (se2->se_col < se1->se_col)
			return(1);
		return(0);
	}
	if (se1->se_type == SAVEDSEL)
		return(-1);
	return(1);
}

