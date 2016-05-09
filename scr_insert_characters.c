#include "scr_insert_characters.h"

#include "cursor.h"
#include "global.h"
#include "jbxvt.h"
#include "screen.h"
#include "selection.h"
#include "xvt.h"

#include <string.h>

//  Insert count spaces from the current position.
void scr_insert_characters(int count)
{
	int x1, x2, y, width, i;
	unsigned char *r, *s;

	if (count > cwidth - screen->col)
		count = cwidth - screen->col;
	if (count <= 0)
		return;

	home_screen();
	cursor();
	check_selection(screen->row,screen->row);
	s = screen->text[screen->row];
	r = screen->rend[screen->row];
	for (i = cwidth - 1; i >= screen->col + count; i--) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
	memset(s + screen->col,0,count);
	memset(r + screen->col,0,count);
	y = MARGIN + screen->row * fheight;
	x1 = MARGIN + screen->col * fwidth;
	x2 = x1 + count * fwidth;
	width = (cwidth - count - screen->col) * fwidth;
	if (width > 0)
		XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.win.vt,jbxvt.X.gc.ne,x1,y,width,fheight,x2,y);
	x1 = MARGIN + screen->col * fwidth;
	width = count * fwidth;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,x1,y,width,fheight,False);
	screen->wrap_next = 0;
	cursor();
}

