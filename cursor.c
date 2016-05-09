#include "cursor.h"

#include "global.h"
#include "jbxvt.h"
#include "screen.h"
#include "xvt.h"

static uint8_t focus;	/* window has the keyboard focus */

//  Draw the cursor at the current position.
void cursor(void)
{
	if (jbxvt.scr.offset > 0)
		return;

	const int x = MARGIN + jbxvt.X.font_width * screen->col;
	const int y = MARGIN + jbxvt.X.font_height * screen->row;
	XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.cu,x,y,jbxvt.X.font_width,jbxvt.X.font_height);
	if (focus == 0)
		XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.cu,x + 1,y + 1,
			jbxvt.X.font_width - 2,jbxvt.X.font_height - 2);
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


