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

	if (count > jbxvt.scr.chars.width - jbxvt.scr.current->col)
		count = jbxvt.scr.chars.width - jbxvt.scr.current->col;
	if (count <= 0)
		return;

	home_screen();
	cursor();
	check_selection(jbxvt.scr.current->row,jbxvt.scr.current->row);
	s = jbxvt.scr.current->text[jbxvt.scr.current->row];
	r = jbxvt.scr.current->rend[jbxvt.scr.current->row];
	for (i = jbxvt.scr.current->col + count; i < jbxvt.scr.chars.width; i++) {
		s[i - count] = s[i];
		r[i - count] = r[i];
	}
	memset(s + jbxvt.scr.chars.width - count,0,count);
	memset(r + jbxvt.scr.chars.width - count,0,count);
	y = MARGIN + jbxvt.scr.current->row * jbxvt.X.font_height;
	x2 = MARGIN + jbxvt.scr.current->col * jbxvt.X.font_width;
	x1 = x2 + count * jbxvt.X.font_width;
	width = (jbxvt.scr.chars.width - count - jbxvt.scr.current->col)
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
	jbxvt.scr.current->wrap_next = 0;
	cursor();
}


