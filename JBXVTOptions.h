// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTOPTIONS_H
#define JBXVT_JBXVTOPTIONS_H
#include <stdint.h>
#include <stdbool.h>
#include "libjb/JBDim.h"
struct JBXVTOptions {
	char * foreground_color, * background_color,
	     * normal_font, * bold_font, * italic_font;
	struct JBDim size;
	struct JBDim position;
	uint8_t screen:7;
	bool show_scrollbar:1;
};
#endif//!JBXVT_JBXVTOPTIONS_H
