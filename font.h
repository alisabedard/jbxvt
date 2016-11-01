// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_FONT_H
#define JBXVT_FONT_H
#include "libjb/size.h"
#include <xcb/xcb.h>
xcb_cursor_t jbxvt_get_cursor(xcb_connection_t * xc, const uint16_t id,
	const uint16_t fg, const uint16_t bg);
uint8_t jbxvt_get_font_ascent(void);
struct JBDim jbxvt_get_font_size(void);
void jbxvt_setup_fonts(xcb_connection_t * xc);
#endif//!JBXVT_FONT_H
