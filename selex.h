/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef JBXVT_SELEX_H
#define JBXVT_SELEX_H
#include <stdbool.h>
#include <xcb/xcb.h>
struct JBDim;
void jbxvt_extend_selection(xcb_connection_t * xc,
	const struct JBDim p, const bool drag);
#endif//!JBXVT_SELEX_H
