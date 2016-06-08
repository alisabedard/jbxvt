/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_tab.h"

#include "cursor.h"
#include "jbxvt.h"
#include "screen.h"
#include "selection.h"

//  Tab to the next tab_stop.
void scr_tab(void)
{
	home_screen();
	if (jbxvt.scr.current->cursor.x == jbxvt.scr.chars.width - 1)
		return;
	cursor(CURSOR_DRAW);
	if (jbxvt.scr.current->text[jbxvt.scr.current->cursor.y]
		[jbxvt.scr.current->cursor.x] == 0)
		jbxvt.scr.current->text[jbxvt.scr.current->cursor.y]
				[jbxvt.scr.current->cursor.x] = '\t';
	jbxvt.scr.current->cursor.x++;
	while (jbxvt.scr.current->cursor.x % 8
		&& jbxvt.scr.current->cursor.x
		< jbxvt.scr.chars.width - 1)
		++jbxvt.scr.current->cursor.x;
	cursor(CURSOR_DRAW);
}

