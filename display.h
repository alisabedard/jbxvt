// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_DISPLAY_H
#define JBXVT_DISPLAY_H
#include "font.h"
#include "libjb/size.h"
#include <stdint.h>
#include <xcb/xcb.h>
uint8_t jbxvt_get_font_ascent(void);
struct JBDim jbxvt_get_font_size(void);
xcb_connection_t * jbxvt_init_display(char * restrict name);
#endif//!JBXVT_DISPLAY_H
