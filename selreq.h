/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#ifndef JBXVT_SELREQ_H
#define JBXVT_SELREQ_H

#include <xcb/xcb.h>

//  Request the current primary selection
void request_selection(const xcb_timestamp_t t);

//  Respond to a notification that a primary selection has been sent
void paste_primary(const xcb_timestamp_t t, const xcb_window_t window,
	const xcb_atom_t property);

#endif//!JBXVT_SELREQ_H
