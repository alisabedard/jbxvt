#ifndef SAVE_SELECTION_H
#define SAVE_SELECTION_H

#include <stdint.h>

/*  Convert the currently marked screen selection as a text string and save it
 *  as the current saved selection.  0 is returned for a success, -1 for a failure.
 */
int8_t save_selection(void);

#endif//!SAVE_SELECTION_H
