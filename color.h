#ifndef JBXVT_COLOR_H
#define JBXVT_COLOR_H

#include <stdint.h>
#include <xcb/xproto.h>

// describe var as relating to a pixel:
typedef uint32_t pixel_t;

// returns pixel value for specified color
pixel_t get_pixel(const char * restrict color)
	__attribute__((nonnull));

// Use rgb color
pixel_t get_pixel_rgb(int16_t r, int16_t g, int16_t b);

pixel_t set_color(const unsigned long vm,
	const pixel_t p, xcb_gcontext_t gc);

// NULL value resets colors to stored value
void set_fg(const char * restrict color); //foreground
void set_bg(const char * restrict color); //background

#endif//!JBXVT_COLOR_H
