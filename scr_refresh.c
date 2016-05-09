#include "scr_refresh.h"

#include "cursor.h"

#include "jbxvt.h"
#include "repaint.h"
#include "screen.h"
#include "xvt.h"

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
	if (col1 >= cwidth)
		col1 = cwidth - 1;
	if (col2 < 0)
		col2 = 0;
	if (col2 >= cwidth)
		col2 = cwidth - 1;
	row1 = (y - MARGIN) / jbxvt.X.font_height;
	row2 = (y + height - MARGIN + jbxvt.X.font_height - 1)
		/ jbxvt.X.font_height - 1;
	if (row1 < 0)
		row1 = 0;
	if (row1 >= cheight)
		row1 = cheight - 1;
	if (row2 < 0)
		row2 = 0;
	if (row2 >= cheight)
		row2 = cheight - 1;
	repaint(row1,row2,col1,col2);
	if (screen->row >= row1 && screen->row <= row2 &&
	    screen->col >= col1 && screen->col <= col2)
		cursor();
}

