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
void scr_refresh(const Point pos, const Size sz)
{
	const Size s = jbxvt.scr.chars;
	const int16_t x = pos.x - MARGIN;
	const int16_t y = pos.y - MARGIN;
	const uint8_t fh = jbxvt.X.font_height;
	const uint8_t fw = jbxvt.X.font_width;

	const Point rc1 = {
		.r = constrain(y / fh, s.h),
		.c = constrain(x / fw, s.w)
	};
	const Point rc2 = {
		.r = constrain((y + sz.height + fh) / fh, s.h),
		.c = constrain((x + sz.width + fw) / fw, s.w)
	};
	LOG("c1: %d, c2: %d, r1: %d, r2: %d\n", rc1.c, rc2.c, rc1.r, rc2.r);
	repaint(rc1, rc2);
}

