#include "repair_damage.h"

#include "global.h"
#include "jbxvt.h"
#include "repaint.h"
#include "screen.h"
#include "xvt.h"

/*  Return true if the event is a graphics exposure or noexposure.
 */
static Bool grexornoex(Display * restrict dpy __attribute__((unused)),
	XEvent * restrict ev, char * restrict arg __attribute__((unused)))
{
	return(ev->type == GraphicsExpose || ev->type == NoExpose);
}

/*  Check for and repair any damage after copying an area of the window.
 */
void repair_damage(void)
{
	XEvent event;
	int row1, row2, col1, col2;

	do {
		/*  Get the next graphics exposure or noexposure event.
		 */
		XIfEvent(jbxvt.X.dpy,&event,grexornoex,NULL);
		if (event.type == NoExpose)
			return;

		row1 = (event.xgraphicsexpose.y - MARGIN)
			/ jbxvt.X.font_height;
		if (row1 < 0)
			row1 = 0;
		row2 = (event.xgraphicsexpose.y
			+ event.xgraphicsexpose.height - MARGIN)
			/ jbxvt.X.font_height;
		if (row2 >= cheight)
			row2 = cheight - 1;
		col1 = (event.xgraphicsexpose.x - MARGIN)
			/ jbxvt.X.font_width;
		if (col1 < 0)
			col1 = 0;
		col2 = (event.xgraphicsexpose.x
			+ event.xgraphicsexpose.width - MARGIN)
			/ jbxvt.X.font_width;
		if (col2 >= cwidth)
			col2 = cwidth - 1;
		repaint(row1,row2,col1,col2);
	} while (event.xgraphicsexpose.count > 0);
}


