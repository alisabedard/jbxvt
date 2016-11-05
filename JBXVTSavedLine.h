/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#ifndef SLINEST_H
#define SLINEST_H
#include "config.h"
#include <stdbool.h>
#include <stdint.h>
//  structure describing a saved line
struct JBXVTSavedLine {
	uint8_t text[JBXVT_MAX_COLUMNS];
	uint32_t rend[JBXVT_MAX_COLUMNS];
	uint16_t size:14;	// line length
	bool wrap:1;		// wrap flag
	bool dwl:1;		// double-width line
};
#endif//!SLINEST_H
