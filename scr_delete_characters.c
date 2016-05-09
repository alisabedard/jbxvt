#include "scr_delete_characters.h"

#include "cursor.h"
#include "global.h"
#include "jbxvt.h"
#include "repair_damage.h"
#include "screen.h"
#include "selection.h"
#include "xvt.h"

#include <string.h>

//  Delete count characters from the current position.
void scr_delete_characters(int count)
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
	for (i = screen->col + count; i < cwidth; i++) {
		s[i - count] = s[i];
		r[i - count] = r[i];
	}
	memset(s + cwidth - count,0,count);
	memset(r + cwidth - count,0,count);
	y = MARGIN + screen->row * fheight;
	x2 = MARGIN + screen->col * fwidth;
	x1 = x2 + count * fwidth;
	width = (cwidth - count - screen->col) * fwidth;
	if (width > 0) {
		XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.win.vt,jbxvt.X.gc.tx,x1,y,width,fheight,x2,y);
		repair_damage();
	}
	x1 = x2 + width;
	width = count * fwidth;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,x1,y,width,fheight,False);
	screen->wrap_next = 0;
	cursor();
}


