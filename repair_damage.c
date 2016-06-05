/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "repair_damage.h"

#include "config.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "screen.h"

#include <stdlib.h>

#if 0
//  Return true if the event is a graphics exposure or noexposure.
static Bool grexornoex(Display * restrict dpy __attribute__((unused)),
	XEvent * restrict ev, char * restrict arg __attribute__((unused)))
{
	return(ev->type == GraphicsExpose || ev->type == NoExpose);
}
#endif

//  Check for and repair any damage after copying an area of the window.
void repair_damage(void)
{
	repaint((xcb_point_t){}, (xcb_point_t){
		.x = jbxvt.scr.chars.width - 1,
		.y = jbxvt.scr.chars.height - 1});
#if 0
	return;
	uint8_t count = 0;
	LOG("repair_damage()");
	do {
		//  Get the next graphics exposure or noexposure event.
		xcb_expose_event_t * e = (xcb_expose_event_t *)
			xcb_poll_for_event(jbxvt.X.xcb);
		if (!e)
			  return;
		switch (e->response_type & ~0x80) {
		case XCB_GRAPHICS_EXPOSURE:
		case XCB_EXPOSE:
			break;
		default:
			free(e);
			return;
		}
		xcb_point_t rc1, rc2;
		rc1.y = constrain((e->y - MARGIN) / jbxvt.X.font_height,
			jbxvt.scr.chars.height);
		rc2.y = constrain((e->y + e->height - MARGIN)
			/ jbxvt.X.font_height, jbxvt.scr.chars.height);
		rc1.x = constrain((e->x - MARGIN)
			/ jbxvt.X.font_width, jbxvt.scr.chars.width);
		rc2.x = constrain((e->x + e->width - MARGIN)
			/ jbxvt.X.font_width, jbxvt.scr.chars.width);
		count = e->count;
		free(e);
		repaint(rc1, rc2);
	} while (count > 0);
#endif
}
