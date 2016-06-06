#ifndef JBXVT_COLOR_H
#define JBXVT_COLOR_H

#include <stdint.h>

// describe var as relating to a pixel:
typedef unsigned long pixel_t;

// returns pixel value for specified color
pixel_t get_pixel(const char * restrict color)
	__attribute__((nonnull));

// NULL value resets colors to stored value
void set_fg(const char * restrict color); //foreground
void set_bg(const char * restrict color); //background

#endif//!JBXVT_COLOR_H
