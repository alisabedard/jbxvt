// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_OPTIONS_H
#define JBXVT_OPTIONS_H
#include "color.h"
#include "font.h"
#include "libjb/JBDim.h"
struct JBXVTOptions {
	struct JBXVTColorOptions color;
	struct JBXVTFontOptions font;
	struct JBDim size;
	struct JBDim position;
	int screen;
	bool show_scrollbar;
};
#endif//!JBXVT_OPTIONS_H
