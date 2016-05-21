/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SAVE_SELECTION_H
#define SAVE_SELECTION_H

#include <stdbool.h>

/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection.  true is returned
    for a success, false for a failure.  */
bool save_selection(void);

#endif//!SAVE_SELECTION_H
