/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#ifndef JBXVT_SELEX_H
#define JBXVT_SELEX_H

#include <stdbool.h>
#include <xcb/xcb.h>

void scr_extend_selection(const xcb_point_t p, const bool drag);

#endif//!JBXVT_SELEX_H

