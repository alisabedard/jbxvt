#include "cursor.h"

#include "global.h"
#include "jbxvt.h"
#include "screen.h"
#include "xvt.h"

static uint8_t focus;	/* window has the keyboard focus */

//  Draw the cursor at the current position.
void cursor(void)
{
	if (offset > 0)
		return;

	const int x = MARGIN + fwidth * screen->col;
	const int y = MARGIN + fheight * screen->row;
	XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.cu,x,y,fwidth,fheight);
	if (focus == 0)
		XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.cu,x + 1,y + 1,
			fwidth - 2,fheight - 2);
}

/*  Indicate a change of keyboard focus.  type is 1 for entry events and 2 for
 *  focus events.
 */
void scr_focus(const uint8_t type, const bool is_in)
{
	cursor();
	if (is_in)
		focus |= type;
	else
		focus &= ~type;
	cursor();
}


