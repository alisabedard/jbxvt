/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_REQ_SEL_H
#define SCR_REQ_SEL_H

#include <xcb/xproto.h>

//  Request the current primary selection
void scr_request_selection(xcb_timestamp_t time, int16_t x, int16_t y);

//  Respond to a notification that a primary selection has been sent
void scr_paste_primary(const xcb_window_t window, const xcb_atom_t property);


#endif//!SCR_REQ_SEL_H
