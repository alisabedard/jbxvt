/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef JBXVT_SELREQ_H
#define JBXVT_SELREQ_H
#include <xcb/xcb.h>
#include <xcb/xproto.h>
//  Request the current primary selection
void jbxvt_request_selection(xcb_connection_t * xc,
    const xcb_timestamp_t t);
//  Respond to a notification that a primary selection has been sent
// Returns the number of bytes read.
uint32_t jbxvt_paste_primary(xcb_connection_t * xc,
    const xcb_timestamp_t t, const xcb_window_t window,
    const xcb_atom_t property);
#endif//!JBXVT_SELREQ_H
