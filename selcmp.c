/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selcmp.h"

#include <stdbool.h>

static int8_t cmp(const int8_t mod, SelEnd * restrict se1, SelEnd * restrict se2)
{
	if (se1->index > se2->index)
		  return - mod;
	if (se1->index < se2->index)
		  return mod;
	return se1->col - se2->col;
}

/*  Compare the two selections and return negtive, 0 or positive depending on
 *  whether se2 is after, equal to or before se1.  */
int8_t selcmp(SelEnd * restrict se1, SelEnd * restrict se2)
{
	const bool se1sv = se1->type == SAVEDSEL;
	if (se1sv && se2->type == SAVEDSEL)
		  return cmp(1, se1, se2);
	if (se1->type == SCREENSEL && se2->type == SCREENSEL)
		  return cmp(-1, se1, se2);
	return se1sv ? -1 : 1;
}

