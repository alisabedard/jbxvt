/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_REQ_SEL_H
#define SCR_REQ_SEL_H

#include <X11/Xlib.h>

//  Request the current primary selection
void scr_request_selection(int time, int16_t x, int16_t y);

//  Respond to a notification that a primary selection has been sent
void scr_paste_primary(const Window window, const Atom property);


#endif//!SCR_REQ_SEL_H
