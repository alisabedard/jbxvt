#include "scr_insert_characters.h"

#include "cursor.h"
#include "jbxvt.h"
#include "screen.h"
#include "selection.h"
#include "xvt.h"

#include <string.h>

//  Insert count spaces from the current position.
void scr_insert_characters(int16_t count)
{
	if (count > jbxvt.scr.chars.width - jbxvt.scr.current->col)
		  count = jbxvt.scr.chars.width - jbxvt.scr.current->col;
	if (count <= 0)
		  return;

	home_screen();
	cursor();
	check_selection(jbxvt.scr.current->row,jbxvt.scr.current->row);
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->row];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->row];
	for (int16_t i = jbxvt.scr.chars.width - 1;
		i >= jbxvt.scr.current->col + count; i--) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
	memset(s + jbxvt.scr.current->col,0,count);
	memset(r + jbxvt.scr.current->col,0,count);
	const XPoint p = { .x = MARGIN + jbxvt.scr.current->col
		* jbxvt.X.font_width, .y = MARGIN + jbxvt.scr.current->row
			* jbxvt.X.font_height};
	const uint16_t width = (jbxvt.scr.chars.width - count
		- jbxvt.scr.current->col) * jbxvt.X.font_width;
	if (width > 0)
		  XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,
			  jbxvt.X.win.vt, jbxvt.X.gc.ne,
			  p.x, p.y, width, jbxvt.X.font_height,
			  p.x+count*jbxvt.X.font_width, p.y);
	XClearArea(jbxvt.X.dpy, jbxvt.X.win.vt, p.x, p.y,
		count * jbxvt.X.font_width, jbxvt.X.font_height, False);
	jbxvt.scr.current->wrap_next = 0;
	cursor();
}

