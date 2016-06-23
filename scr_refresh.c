/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_refresh.h"

#include "config.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"

__attribute__((const))
static uint16_t constrain(uint16_t rc, const int16_t lim)
{
	rc = MAX(rc, 0);
	rc = MIN(rc, lim);
	return rc;
}

/*  Refresh the region of the screen delimited by the aruments.  Used to
 *  repair after minor exposure events.  */
void scr_refresh(xcb_rectangle_t box)
{
	LOG("scr_refresh()");
	const Size s = jbxvt.scr.chars;
	const Size f = {.h = jbxvt.X.font_height, .w = jbxvt.X.font_width};
	box.x -= MARGIN;
	box.y -= MARGIN;
	const xcb_point_t rc1 = {
		.x = constrain(box.x / f.w, s.w),
		.y = constrain(box.y / f.h, s.h)};
	const xcb_point_t rc2 = {
		.x = constrain((box.x + box.width + f.w) / f.w, s.w),
		.y = constrain((box.y + box.height + f.h) / f.h, s.h)};
	repaint(rc1, rc2);
}

