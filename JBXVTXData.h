// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTXDATA_H
#define JBXVT_JBXVTXDATA_H
#include "libjb/xcb.h"
struct JBXVTXWindows {
	xcb_window_t vt, sb, main;
};
struct JBXVTXGCs {
	xcb_gcontext_t tx, cu;
};
struct JBXVTXPixels {
	pixel_t bg, fg, current_fg, current_bg;
};
struct JBXVTFontData {
	xcb_font_t normal, bold, italic;
	struct JBDim size;
	int8_t ascent;
};
struct JBXVTXData {
	xcb_connection_t * xcb;
	xcb_screen_t * screen;
	struct JBXVTXWindows win;
	struct JBXVTXGCs gc;
	struct JBXVTXPixels color;
	struct JBXVTFontData font;
};
#endif//!JBXVT_JBXVTXDATA_H
