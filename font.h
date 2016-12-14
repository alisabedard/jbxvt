// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_NORMAL_FONT_H
#define JBXVT_NORMAL_FONT_H
#include <xcb/xcb.h>
struct JBDim;
struct JBXVTFontOptions {
	char * normal;
	char * bold;
	char * italic;
};
uint8_t jbxvt_get_font_ascent(void);
struct JBDim jbxvt_get_font_size(void);
xcb_font_t jbxvt_get_normal_font(xcb_connection_t * xc);
xcb_font_t jbxvt_get_bold_font(xcb_connection_t * xc);
xcb_font_t jbxvt_get_italic_font(xcb_connection_t * xc);
void jbxvt_init_fonts(xcb_connection_t * xc,
	struct JBXVTFontOptions * opt);
#endif//!JBXVT_NORMAL_FONT_H
