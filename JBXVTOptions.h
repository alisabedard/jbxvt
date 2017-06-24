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
	uint8_t screen:7;
	bool show_scrollbar:1;
#ifdef JBXVT_USE_PADDED
	int8_t __pad[7]; // pad to alignment boundary
};
#else//!JBXVT_USE_PADDED
} __attribute__((packed)); // saves 7 bytes
#endif//JBXVT_USE_PADDED
#endif//!JBXVT_JBXVTOPTIONS_H
