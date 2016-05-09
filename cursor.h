#ifndef JBXVT_CURSOR_H
#define JBXVT_CURSOR_H

#include <stdbool.h>
#include <stdint.h>

//  Draw the cursor at the current position.
void cursor(void);

/*  Indicate a change of keyboard focus.  type is 1 for entry events and 2 for
 *  focus events.
 */
void scr_focus(const uint8_t type, const bool is_in);

#endif//!JBXVT_CURSOR_H
