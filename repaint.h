/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_REPAINT_H
#define JBXVT_REPAINT_H
#include <xcb/xcb.h>
// Repaint the screen
void jbxvt_repaint(xcb_connection_t * xc);
#endif//JBXVT_REPAINT_H
