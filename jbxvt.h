// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_H
#define JBXVT_H
#include "libjb/util.h"
#include "selection.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTSavedLine.h"
#include "JBXVTEvent.h"
#include "JBXVTScreen.h"
struct JBXVTScreenSavedLines {
	struct JBXVTSavedLine data[JBXVT_MAX_SCROLL]; // saved lines
	uint16_t top, max;
};
struct JBXVTScreenData {
	struct JBXVTScreen * current, * s;
	struct JBXVTScreenSavedLines sline;
	struct JBDim chars;
	struct JBDim pixels;
	uint32_t rstyle; // render style
};
struct JBXVT {
	struct JBXVTScreenData scr;
	struct JBXVTPrivateModes mode;
};
extern struct JBXVT jbxvt; // in jbxvt.c
#endif//!JBXVT_H
