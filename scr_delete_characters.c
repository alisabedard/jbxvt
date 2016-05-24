/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_delete_characters.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "repair_damage.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

//  Delete count characters from the current position.
void scr_delete_characters(int count)
{
	count = constrain(count, jbxvt.scr.chars.width
		- jbxvt.scr.current->col);
	if(!count)
		  return;

	home_screen();
	cursor(CURSOR_DRAW);
	check_selection(jbxvt.scr.current->row,jbxvt.scr.current->row);
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->row];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->row];
	for (uint8_t i = jbxvt.scr.current->col + count;
		i < jbxvt.scr.chars.width; i++) {
		s[i - count] = s[i];
		r[i - count] = r[i];
	}
	memset(s + jbxvt.scr.chars.width - count,0,count);
	memset(r + jbxvt.scr.chars.width - count,0,count);
	int16_t y = MARGIN + jbxvt.scr.current->row * jbxvt.X.font_height;
	int16_t x2 = MARGIN + jbxvt.scr.current->col * jbxvt.X.font_width;
	int16_t x1 = x2 + count * jbxvt.X.font_width;
	uint16_t width = (jbxvt.scr.chars.width - count - jbxvt.scr.current->col)
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
	cursor(CURSOR_DRAW);
}

