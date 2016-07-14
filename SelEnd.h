// Copyright 2016, Jeffrey E. Bedard

#ifndef SELEND_H
#define SELEND_H

#include <stdint.h>

//  selection endpoint types.
typedef enum {
	NOSEL,
	SCREENSEL,
	SAVEDSEL
} SelType;

//  structure describing a selection endpoint.
typedef struct {
	SelType type;
	int16_t index;	// index into the sline or screen array
	uint8_t col;	// column of the character
} SelEnd;

#endif//!SELEND_H
