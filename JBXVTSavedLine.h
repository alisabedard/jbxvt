/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#ifndef SLINEST_H
#define SLINEST_H

#include <stdbool.h>
#include <stdint.h>

//  structure describing a saved line
struct JBXVTSavedLine {
	uint8_t *sl_text;	/* the text of the line */
	uint32_t *sl_rend;	/* the rendition style */
	uint16_t sl_length:15;	/* length of the line */
	bool wrap:1;		// wrap flag
};

#endif//!SLINEST_H
