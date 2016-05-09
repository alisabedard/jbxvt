#include "change_offset.h"

#include "cursor.h"
#include "global.h"
#include "jbxvt.h"
#include "repaint.h"
#include "repair_damage.h"
#include "sbar.h"
#include "screen.h"
#include "xvt.h"

// Text has moved down by less than a screen, render lines that remain
static void copy_repaint_repair(const int16_t d, const int16_t y1,
	const int16_t y2, const int row1, const int row2)
{
	const uint16_t height = (cheight - d) * fheight;
	XCopyArea(jbxvt.X.dpy, jbxvt.X.win.vt, jbxvt.X.win.vt, jbxvt.X.gc.tx, 0, y1,
		pwidth, height, 0, y2);
	repaint(row1, row2, 0, cwidth-1);
	repair_damage();
}

//  Change the value of the scrolled screen offset and repaint the screen
void change_offset(int16_t n)
{
	if (n > sline_top)
		n = sline_top;
	if (n < 0)
		n = 0;
	if (n == offset)
		return;
	cursor();
	int16_t d = n - offset;
	offset = n;
	if (d > 0 && d < cheight) {
		/*  Text has moved down by less than a screen so raster
		 *  the lines that did not move off.
		 */
		copy_repaint_repair(d, MARGIN, MARGIN + d * fheight,
			0, d - 1);
	} else if (d < 0 && -d < cheight) {
		/*  Text has moved down by less than a screen so raster
		 *  the lines that did not move off.
		 */
		d = -d;
		copy_repaint_repair(d, MARGIN + d * fheight, MARGIN,
			cheight - d, cheight - 1);
	} else
		repaint(0,cheight - 1,0,cwidth - 1);
	cursor();
	// Update current scrollbar position due to change
	sbar_show(cheight + sline_top - 1, offset, offset + cheight - 1);
}


