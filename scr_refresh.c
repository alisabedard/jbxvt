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
	const uint16_t col1 = constrain(x/jbxvt.X.font_width,
		jbxvt.scr.chars.width);
	const uint16_t col2 = constrain((x+width+jbxvt.X.font_width)
		/jbxvt.X.font_width,
		jbxvt.scr.chars.width);
	const uint16_t row1 = constrain(y/jbxvt.X.font_height,
		jbxvt.scr.chars.height);
	const uint16_t row2 = constrain((y+height+jbxvt.X.font_height)
		/jbxvt.X.font_height,
		jbxvt.scr.chars.height);
	LOG("c1: %d, c2: %d, r1: %d, r2: %d\n", col1, col2, row1, row2);
	repaint(row1,row2,col1,col2);
}

