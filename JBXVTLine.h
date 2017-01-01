/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef JBXVT_JBXVTLINE_H
#define JBXVT_JBXVTLINE_H
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "rstyle.h"
//  structure describing a saved line
struct JBXVTLine {
	uint8_t text[JBXVT_MAX_COLUMNS];
	rstyle_t rend[JBXVT_MAX_COLUMNS];
	uint16_t size:14;	// line length
	bool wrap:1;		// wrap flag
	bool dwl:1;		// double-width line
};
#endif//!JBXVT_JBXVTLINE_H
