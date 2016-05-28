/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "repair_damage.h"

#include "config.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "screen.h"

//  Return true if the event is a graphics exposure or noexposure.
static Bool grexornoex(Display * restrict dpy __attribute__((unused)),
	XEvent * restrict ev, char * restrict arg __attribute__((unused)))
{
	return(ev->type == GraphicsExpose || ev->type == NoExpose);
}

//  Check for and repair any damage after copying an area of the window.
void repair_damage(void)
{
	LOG("repair_damage()");
	XEvent event;
	do {
		//  Get the next graphics exposure or noexposure event.
		XIfEvent(jbxvt.X.dpy,&event,grexornoex,NULL);
		if (event.type == NoExpose)
			return;

		Point rc1, rc2;
		rc1.r = constrain((event.xgraphicsexpose.y - MARGIN)
			/ jbxvt.X.font_height, jbxvt.scr.chars.height);
		rc2.r = constrain((event.xgraphicsexpose.y
			+ event.xgraphicsexpose.height - MARGIN)
			/ jbxvt.X.font_height, jbxvt.scr.chars.height);
		rc1.c = constrain((event.xgraphicsexpose.x - MARGIN)
			/ jbxvt.X.font_width, jbxvt.scr.chars.width);
		rc2.c = constrain((event.xgraphicsexpose.x
			+ event.xgraphicsexpose.width - MARGIN)
			/ jbxvt.X.font_width, jbxvt.scr.chars.width);
		repaint(rc1, rc2);
	} while (event.xgraphicsexpose.count > 0);
}
