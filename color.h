#ifndef JBXVT_COLOR_H
#define JBXVT_COLOR_H

#include <stdint.h>

// describe var as relating to a pixel:
typedef unsigned long pixel_t;

// returns pixel value for specified color

pixel_t get_pixel(const char * restrict color)
	__attribute__((nonnull));

// Reset colors to stored background and foreground.
void reset_color(void);

void reset_fg(void);
void reset_bg(void);

void set_fg(const char * color); //foreground
void set_bg(const char * color); //background

#endif//!JBXVT_COLOR_H
