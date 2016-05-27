/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_DIM_H
#define JBXVT_DIM_H

#include <stdint.h>

// Structure for dimensional data:
typedef struct Dim {
	union {
		int16_t x;
		int16_t r; // row
		int16_t row;
		int16_t h; // height
		uint16_t height;
		uint16_t top;
		uint16_t t; // top
	};
	union {
		int16_t y;
		int16_t c; // column
		int16_t col; // column
		int16_t column;
		uint16_t w; // width
		uint16_t width;
		uint16_t bottom;
		uint16_t bot; // bottom
		uint16_t b; // bottom
	};
} Dim;

#endif//!JBXVT_DIM_H
