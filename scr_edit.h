/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_EDIT_H
#define SCR_EDIT_H

#include <stdint.h>

//  Insert count spaces from the current position.
void scr_insert_characters(int8_t count);

//  Delete count characters from the current position.
void scr_delete_characters(int8_t count);

#endif//!SCR_EDIT_H
