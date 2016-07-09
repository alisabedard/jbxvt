/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_REQ_SEL_H
#define SCR_REQ_SEL_H

#include <xcb/xcb.h>

//  Request the current primary selection
void request_selection(const xcb_timestamp_t t);

//  Respond to a notification that a primary selection has been sent
void paste_primary(const xcb_window_t window, const xcb_atom_t property);


#endif//!SCR_REQ_SEL_H
