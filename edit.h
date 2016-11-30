/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_EDIT_H
#define JBXVT_EDIT_H
#include <xcb/xcb.h>
//  Insert count spaces from the current position.
void jbxvt_insert_characters(xcb_connection_t * xc, const uint8_t count);
//  Delete count characters from the current position.
void jbxvt_delete_characters(xcb_connection_t * xc, const uint8_t count);
#endif//!JBXVT_EDIT_H
