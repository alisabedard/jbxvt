/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_EXTEND_SELECTION_H
#define SCR_EXTEND_SELECTION_H

#include <stdbool.h>
#include <xcb/xproto.h>

void scr_extend_selection(const xcb_point_t p, const bool drag);

#endif//!SCR_EXTEND_SELECTION_H

