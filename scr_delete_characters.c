#include "scr_delete_characters.h"

#include "cursor.h"

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

	if (count > jbxvt.scr.chars.width - screen->col)
		count = jbxvt.scr.chars.width - screen->col;
	if (count <= 0)
		return;

	home_screen();
	cursor();
	check_selection(screen->row,screen->row);
	s = screen->text[screen->row];
	r = screen->rend[screen->row];
	for (i = screen->col + count; i < jbxvt.scr.chars.width; i++) {
		s[i - count] = s[i];
		r[i - count] = r[i];
	}
	memset(s + jbxvt.scr.chars.width - count,0,count);
	memset(r + jbxvt.scr.chars.width - count,0,count);
	y = MARGIN + screen->row * jbxvt.X.font_height;
	x2 = MARGIN + screen->col * jbxvt.X.font_width;
	x1 = x2 + count * jbxvt.X.font_width;
	width = (jbxvt.scr.chars.width - count - screen->col)
		* jbxvt.X.font_width;
	if (width > 0) {
		XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.win.vt,
			jbxvt.X.gc.tx,x1,y,width,jbxvt.X.font_height,x2,y);
		repair_damage();
	}
	x1 = x2 + width;
	width = count * jbxvt.X.font_width;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,x1,y,width,
		jbxvt.X.font_height,False);
	screen->wrap_next = 0;
	cursor();
}


