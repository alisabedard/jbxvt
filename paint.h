
/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_PAINT_H
#define JBXVT_PAINT_H

#include <stdint.h>
#include <xcb/xproto.h>

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, xcb_point_t p);

#endif//!JBXVT_PAINT_H
