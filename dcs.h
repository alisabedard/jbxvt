/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_DCS_H
#define JBXVT_DCS_H
#include <xcb/xcb.h>
struct JBXVTToken;
void jbxvt_dcs(xcb_connection_t * xc, struct JBXVTToken * t);
#endif//!JBXVT_DCS_H
