/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_REPAINT_H
#define JBXVT_REPAINT_H

#include <stdint.h>

/* Repaint the box delimited by row1 to row2 and col1 to col2 of the displayed
 * screen from the backup screen.  */
void repaint(int row1, int row2, int col1, int col2);

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, int16_t x, int16_t y);

#endif//JBXVT_REPAINT_H
