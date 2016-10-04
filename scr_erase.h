/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_SCR_ERASE_H
#define JBXVT_SCR_ERASE_H

#include <stdint.h>

//  erase part or the whole of a line
void jbxvt_erase_line(const int8_t mode);

//  erase part or the whole of the screen
void jbxvt_erase_screen(const int8_t mode);

#endif//!JBXVT_SCR_ERASE_H
