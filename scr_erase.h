/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_ERASE_H
#define SCR_ERASE_H

#include <stdint.h>

//  erase part or the whole of a line
void scr_erase_line(const int8_t mode);

//  erase part or the whole of the screen
void scr_erase_screen(const int8_t mode);

#endif//!SCR_ERASE_H
