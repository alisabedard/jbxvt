#ifndef CHANGE_SELECTION_H
#define CHANGE_SELECTION_H

#include "selst.h"

/*  Repaint the displayed selection to reflect the new value.  ose1 and ose2
 *  are assumed to represent the currently displayed selection endpoints.
 */
void change_selection(SelEnd * restrict ose1, SelEnd * restrict ose2);

#endif//!CHANGE_SELECTION_H
