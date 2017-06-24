// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTOPTIONS_H
#define JBXVT_JBXVTOPTIONS_H
#include <stdint.h>
#include <stdbool.h>
#include "libjb/JBDim.h"
struct JBXVTOptions {
	char * foreground_color, * background_color,
	     * normal_font, * bold_font, * italic_font;
	struct JBDim size, position;
	int32_t screen;
	bool show_scrollbar;
	int __pad:24; // pad to alignment boundary
};
#endif//!JBXVT_JBXVTOPTIONS_H
