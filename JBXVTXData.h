// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTXDATA_H
#define JBXVT_JBXVTXDATA_H
#include "libjb/xcb.h"
struct JBXVTXWindows {
	xcb_window_t vt;
	xcb_window_t main;
};
struct JBXVTXGCs {
	xcb_gcontext_t tx;
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
	struct JBXVTXWindows win;
	struct JBXVTXGCs gc;
	struct JBXVTXPixels color;
	struct JBXVTFontData font;
};
#endif//!JBXVT_JBXVTXDATA_H
