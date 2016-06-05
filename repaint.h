/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_REPAINT_H
#define JBXVT_REPAINT_H

#include "jbxvt.h"

#include <stdint.h>
#include <xcb/xproto.h>

/* Repaint the box delimited by rc1.r to rc2.r and rc1.c to rc2.c
   of the displayed screen from the backup screen.  */
void repaint(xcb_point_t rc1, xcb_point_t rc2);

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(uint8_t * restrict str, uint32_t rval,
	uint8_t len, xcb_point_t p);

#endif//JBXVT_REPAINT_H
