// Copyright 2016, Jeffrey E. Bedard

#ifndef JBXVT_COLOR_H
#define JBXVT_COLOR_H

#include "libjb/xcb.h"

// returns pixel value for specified color
pixel_t get_pixel(const char * restrict color)
	__attribute__((nonnull));

// Use rgb color
pixel_t get_pixel_rgb(const uint16_t r, const uint16_t g, const uint16_t b);

pixel_t set_color(const uint32_t vm, const pixel_t p, const xcb_gcontext_t gc);

// NULL value resets colors to stored value
void set_fg(const char * restrict color); //foreground
void set_bg(const char * restrict color); //background

#endif//!JBXVT_COLOR_H
