/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_refresh.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "repaint.h"
#include "screen.h"

/*  Refresh the region of the screen delimited by the aruments.  Used to
 *  repair after minor exposure events.
 */
void scr_refresh(int x, int y, int width, int height)
{
	int row1, row2, col1, col2;

	col1 = (x - MARGIN) / jbxvt.X.font_width;
	col2 = (x + width - MARGIN + jbxvt.X.font_width - 1)
		/ jbxvt.X.font_width - 1;
	if (col1 < 0)
		col1 = 0;
	if (col1 >= jbxvt.scr.chars.width)
		col1 = jbxvt.scr.chars.width - 1;
	if (col2 < 0)
		col2 = 0;
	if (col2 >= jbxvt.scr.chars.width)
		col2 = jbxvt.scr.chars.width - 1;
	row1 = (y - MARGIN) / jbxvt.X.font_height;
	row2 = (y + height - MARGIN + jbxvt.X.font_height - 1)
		/ jbxvt.X.font_height - 1;
	if (row1 < 0)
		row1 = 0;
	if (row1 >= jbxvt.scr.chars.height)
		row1 = jbxvt.scr.chars.height - 1;
	if (row2 < 0)
		row2 = 0;
	if (row2 >= jbxvt.scr.chars.height)
		row2 = jbxvt.scr.chars.height - 1;
	repaint(row1,row2,col1,col2);
	if (jbxvt.scr.current->row >= row1 && jbxvt.scr.current->row <= row2 &&
	    jbxvt.scr.current->col >= col1 && jbxvt.scr.current->col <= col2)
		cursor(CURSOR_DRAW);
}

