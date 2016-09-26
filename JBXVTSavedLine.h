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
	uint8_t text[JBXVT_MAX_COLS];
	uint32_t rend[JBXVT_MAX_COLS];
	uint16_t sl_length:15;	/* length of the line */
	bool wrap:1;		// wrap flag
};

#endif//!SLINEST_H
