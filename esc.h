/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_ESC_H
#define JBXVT_ESC_H
#include "JBXVTToken.h"
#include <xcb/xcb.h>
void jbxvt_csi(xcb_connection_t * xc,
	int_fast16_t c, struct JBXVTToken * restrict tk);
void jbxvt_end_cs(xcb_connection_t * xc,
	int_fast16_t c, struct JBXVTToken * restrict tk);
void jbxvt_esc(xcb_connection_t * xc,
	int_fast16_t c, struct JBXVTToken * restrict tk);
#endif//!JBXVT_ESC_H
