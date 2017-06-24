// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTSELECTIONDATA_H
#define JBXVT_JBXVTSELECTIONDATA_H
#include <stdbool.h>
#include <stdint.h>
#include "JBXVTSelectionUnit.h"
#include "libjb/JBDim.h"
struct JBXVTSelectionData {
	uint8_t * text;
	struct JBDim end[3]; // end0, end1, anchor
	enum JBXVTSelectionUnit unit;
	int32_t length;
	uint32_t on_screen; /* Large type used for padding
			       to alignment boundary.  */
};
#endif//!JBXVT_JBXVTSELECTIONDATA_H
