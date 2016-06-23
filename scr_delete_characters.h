/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_DEL_CHARS_H
#define SCR_DEL_CHARS_H

#include <stdint.h>

//  Delete count characters from the current position.
void scr_delete_characters(uint8_t count);

#endif//SCR_DEL_CHARS_H
