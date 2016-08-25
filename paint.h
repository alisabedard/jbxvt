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
pixel_t set_fg(const char * color);

// NULL value resets colors to stored value
pixel_t set_bg(const char * color);

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	int16_t len, struct JBDim p);

#endif//!JBXVT_PAINT_H
