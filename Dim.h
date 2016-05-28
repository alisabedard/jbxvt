/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_DIM_H
#define JBXVT_DIM_H

#include <stdint.h>

typedef struct Point {
	union {
		int16_t x;
		int16_t top;
		int16_t column;
		int16_t col;
		int16_t c;
	};
	union {
		int16_t y;
		int16_t bottom;
		int16_t row;
		int16_t r;
	};
} Point;

typedef struct Size {
	union {
		uint16_t width;
		uint16_t w;
	};
	union {
		uint16_t height;
		uint16_t h;
	};
} Size;

typedef struct Rect {
	Point pos;
	Size sz;
} Rect;

#endif//!JBXVT_DIM_H
