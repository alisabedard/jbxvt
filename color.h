// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_COLOR_H
#define JBXVT_COLOR_H
#include <xcb/xcb.h>
#include "libjb/xcb.h"
struct JBXVTOptions;
// Set up initial color values for foreground and background pixels.
void jbxvt_init_colors(xcb_connection_t * xc,
    struct JBXVTOptions * opt);
pixel_t jbxvt_get_bg(void);
pixel_t jbxvt_get_fg(void);
void jbxvt_reverse_screen_colors(xcb_connection_t * xc);
// NULL value resets colors to stored value
pixel_t jbxvt_set_fg(xcb_connection_t * xc, const char * color);
pixel_t jbxvt_set_fg_pixel(xcb_connection_t * xc, const pixel_t p);
// NULL value resets colors to stored value
pixel_t jbxvt_set_bg(xcb_connection_t * xc, const char * color);
pixel_t jbxvt_set_bg_pixel(xcb_connection_t * xc, const pixel_t p);
void jbxvt_set_reverse_video(xcb_connection_t * xc);
#endif//!JBXVT_COLOR_H
