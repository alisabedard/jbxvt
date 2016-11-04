// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_JBXVT_H
#define JBXVT_JBXVT_H
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
struct JBXVT {
	struct JBXVTPrivateModes mode;
};
extern struct JBXVT jbxvt; // in jbxvt.c
#endif//!JBXVT_JBXVT_H
