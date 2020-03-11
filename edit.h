/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_EDIT_H
#define JBXVT_EDIT_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
// Insert or delete count spaces from the current position.
void jbxvt_edit_characters(xcb_connection_t * xc,
    const uint8_t count, const bool delete);
#endif//!JBXVT_EDIT_H
