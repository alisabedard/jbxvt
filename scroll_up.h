/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCROLL_UP_H
#define SCROLL_UP_H

#include <stdint.h>

/*  Scroll jbxvt.scr.s1 up by count lines saving lines as needed.  This is used
 *  after the screen size is reduced.  */
void scroll_up(uint16_t count);

#endif//!SCROLL_UP_H
