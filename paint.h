/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_PAINT_H
#define JBXVT_PAINT_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>
#include "libjb/size.h"
#include "libjb/xcb.h"
// NULL value resets colors to stored value
pixel_t jbxvt_set_fg(xcb_connection_t * xc, const char * color);
// NULL value resets colors to stored value
pixel_t jbxvt_set_bg(xcb_connection_t * xc, const char * color);
//  Paint the text using the rendition value at the screen position.
void jbxvt_paint(xcb_connection_t * xc, uint8_t * restrict str,
	uint32_t rstyle, uint16_t len, struct JBDim p, const bool dwl);
#endif//!JBXVT_PAINT_H
