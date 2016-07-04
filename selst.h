#ifndef SELST_H
#define SELST_H

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

#endif//!SELST_H
