#include "scr_insert_characters.h"

#include "cursor.h"
#include "jbxvt.h"
#include "screen.h"
#include "selection.h"
#include "xvt.h"

#include <string.h>

//  Insert count spaces from the current position.
void scr_insert_characters(int count)
{
	int x1, x2, y, width, i;

	if (count > jbxvt.scr.chars.width - jbxvt.scr.current->col)
		count = jbxvt.scr.chars.width - jbxvt.scr.current->col;
	if (count <= 0)
		return;

	home_screen();
	cursor();
	check_selection(jbxvt.scr.current->row,jbxvt.scr.current->row);
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->row];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->row];
	for (i = jbxvt.scr.chars.width - 1;
		i >= jbxvt.scr.current->col + count; i--) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
	memset(s + jbxvt.scr.current->col,0,count);
	memset(r + jbxvt.scr.current->col,0,count);
	y = MARGIN + jbxvt.scr.current->row * jbxvt.X.font_height;
	x1 = MARGIN + jbxvt.scr.current->col * jbxvt.X.font_width;
	x2 = x1 + count * jbxvt.X.font_width;
	width = (jbxvt.scr.chars.width - count - jbxvt.scr.current->col)
		* jbxvt.X.font_width;
	if (width > 0)
		XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.win.vt,
			jbxvt.X.gc.ne,x1,y,width,jbxvt.X.font_height,x2,y);
	x1 = MARGIN + jbxvt.scr.current->col * jbxvt.X.font_width;
	width = count * jbxvt.X.font_width;
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,x1,y,width,
		jbxvt.X.font_height,False);
	jbxvt.scr.current->wrap_next = 0;
	cursor();
}

