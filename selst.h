#ifndef SELST_H
#define SELST_H

#include <stdint.h>

//  selection endpoint types.
enum seltype {
	NOSEL,
	SCREENSEL,
	SAVEDSEL
};

//  structure describing a selection endpoint.
typedef struct selst {
	enum seltype se_type;
	int16_t se_index;	// index into the sline or screen array
	uint8_t se_col;		// column of the character
} SelEnd;

#endif//!SELST_H
