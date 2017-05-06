/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_PAINT_H
#define JBXVT_PAINT_H
#include <stdbool.h>
#include <xcb/xcb.h>
struct JBDim;
//  Paint the text using the rendition value at the screen position.
void jbxvt_paint(xcb_connection_t * xc, uint8_t * restrict str,
	uint32_t rstyle, uint16_t len, struct JBDim p, const bool dwl);
#endif//!JBXVT_PAINT_H
