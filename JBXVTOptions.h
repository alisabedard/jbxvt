// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_OPTIONS_H
#define JBXVT_OPTIONS_H
#include "JBXVTColorOptions.h"
#include "JBXVTFontOptions.h"
#include "libjb/JBDim.h"
struct JBXVTOptions {
	struct JBXVTColorOptions color;
	struct JBXVTFontOptions font;
	struct JBDim size;
	struct JBDim position;
	uint8_t screen:7;
	bool show_scrollbar:1;
};
#endif//!JBXVT_OPTIONS_H
