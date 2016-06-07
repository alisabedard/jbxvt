/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_refresh.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "screen.h"

// constrain rc between 0 and lim, return new value
__attribute__((const))
static uint_fast32_t constrain(const int_fast32_t rc, const int_fast32_t lim)
{
	if (rc < 0) // guarantee unsigned
		  return 0;
	else if (rc >= lim) // cap
		  return lim - 1;
	return rc; // falls within range
}

/*  Refresh the region of the screen delimited by the aruments.  Used to
 *  repair after minor exposure events.  */
void scr_refresh(const xcb_point_t pos, const Size sz)
{
	LOG("scr_refresh()");
	const Size s = jbxvt.scr.chars;
	const Size f = {.h = jbxvt.X.font_height, .w = jbxvt.X.font_width};
	const int16_t x = pos.x - MARGIN;
	const int16_t y = pos.y - MARGIN;
	const xcb_point_t rc1 = {
		.x = constrain(x / f.w, s.w),
		.y = constrain(y / f.h, s.h)};
	const xcb_point_t rc2 = {
		.x = constrain((x + sz.width + f.w) / f.w, s.w),
		.y = constrain((y + sz.height + f.h) / f.h, s.h)};
	repaint(rc1, rc2);
}

