// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_H
#define JBXVT_H
#include "libjb/util.h"
#include "selection.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTSavedLine.h"
#include "JBXVTEvent.h"
#include "JBXVTScreen.h"
struct JBXVTScreenData {
	struct JBXVTScreen * current, * s;
	struct JBXVTSavedLine saved_lines[JBXVT_MAX_SCROLL];
};
struct JBXVT {
	struct JBXVTScreenData scr;
	struct JBXVTPrivateModes mode;
};
extern struct JBXVT jbxvt; // in jbxvt.c
#endif//!JBXVT_H
