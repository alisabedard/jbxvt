/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_refresh.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "screen.h"

/*  Refresh the region of the screen delimited by the aruments.  Used to
 *  repair after minor exposure events.  */
void scr_refresh(int x, int y, int width, int height)
{
	x -= MARGIN;
	y -= MARGIN;
	const Dim rc1 = {
		.r = constrain(y/jbxvt.X.font_height,
			jbxvt.scr.chars.height),
		.c = constrain(x/jbxvt.X.font_width,
			jbxvt.scr.chars.width)
	};
	const Dim rc2 = {
		.r = constrain((y+height+jbxvt.X.font_height)
			/jbxvt.X.font_height,
			jbxvt.scr.chars.height),
		.c = constrain((x+width+jbxvt.X.font_width)
			/jbxvt.X.font_width,
			jbxvt.scr.chars.width)
	};
	LOG("c1: %d, c2: %d, r1: %d, r2: %d\n", rc1.c, rc2.c, rc1.r, rc2.r);
	repaint(rc1, rc2);
}

