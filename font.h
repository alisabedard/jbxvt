// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_NORMAL_FONT_H
#define JBXVT_NORMAL_FONT_H
#include "libjb/size.h"
#include <xcb/xcb.h>
struct JBXVTFontOptions {
	char * normal;
	char * bold;
	char * italic;
};
// Returns 0 if cursor font cannot be opened.
xcb_cursor_t jbxvt_get_cursor(xcb_connection_t * xc,
	const uint16_t id, const uint16_t fg, const uint16_t bg);
uint8_t jbxvt_get_font_ascent(void);
struct JBDim jbxvt_get_font_size(void);
xcb_font_t jbxvt_get_normal_font(xcb_connection_t * xc);
xcb_font_t jbxvt_get_bold_font(xcb_connection_t * xc);
xcb_font_t jbxvt_get_italic_font(xcb_connection_t * xc);
void jbxvt_init_fonts(xcb_connection_t * xc,
	struct JBXVTFontOptions * opt);
#endif//!JBXVT_NORMAL_FONT_H
