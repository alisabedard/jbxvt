// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_H
#define JBXVT_H
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
struct JBXVTScreenData {
	struct JBXVTScreen * current, * s;
};
struct JBXVT {
	struct JBXVTScreenData scr;
	struct JBXVTPrivateModes mode;
};
extern struct JBXVT jbxvt; // in jbxvt.c
#endif//!JBXVT_H
