/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_PAINT_H
#define JBXVT_PAINT_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>
#include "libjb/xcb.h"

// NULL value resets colors to stored value
void set_fg_or_bg(const char * color, const bool is_fg);
#define set_fg(color) set_fg_or_bg(color, true)
#define set_bg(color) set_fg_or_bg(color, false)

// returns pixel value for specified color
pixel_t get_pixel(const char * restrict color)
	__attribute__((nonnull));

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, xcb_point_t p);

#endif//!JBXVT_PAINT_H
