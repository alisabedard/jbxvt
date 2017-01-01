// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTSELECTIONDATA_H
#define JBXVT_JBXVTSELECTIONDATA_H
#include <stdbool.h>
#include <stdint.h>
#include "JBXVTSelectionUnit.h"
#include "libjb/JBDim.h"
struct JBXVTSelectionData {
	uint8_t * text;
	enum JBXVTSelectionUnit unit;
	struct JBDim end[3]; // end0, end1, anchor
	uint16_t length:15;
	bool on_screen:1;
};
#endif//!JBXVT_JBXVTSELECTIONDATA_H
