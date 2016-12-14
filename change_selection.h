// Copyright 2017, Jeffrey E. Bedard <jefbed@gmail.com>
#ifndef JBXVT_CHANGE_SELECTION_H
#define JBXVT_CHANGE_SELECTION_H
#include <xcb/xcb.h>
struct JBDim;
/*  Repaint the displayed selection to reflect
    the new value.  ose1 and ose2 are assumed
    to represent the currently displayed selection
    endpoints.  */
void jbxvt_change_selection(xcb_connection_t * xc,
	struct JBDim * restrict ose0,
	struct JBDim * restrict ose1);
#endif//!JBXVT_CHANGE_SELECTION_H
