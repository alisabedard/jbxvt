/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#ifndef SHOW_SELECTION_H
#define SHOW_SELECTION_H

#include <stdint.h>

/*  Paint any part of the selection that is
    between rows row1 and row2 inclusive
    and between cols col1 and col2 inclusive.  */
void show_selection(int16_t row1, int16_t row2,
	int16_t col1, int16_t col2);

#endif//!SHOW_SELECTION_H
