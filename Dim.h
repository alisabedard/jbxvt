/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_DIM_H
#define JBXVT_DIM_H

#include <stdint.h>

typedef struct Size {
	union {
		uint16_t width;
		uint16_t w;
		// for scroll margin:
		int16_t top;
		int16_t t;
	};
	union {
		uint16_t height;
		uint16_t h;
		// for scroll margin:
		int16_t bottom;
		int16_t bot;
		int16_t b;
	};
} Size;

#endif//!JBXVT_DIM_H
