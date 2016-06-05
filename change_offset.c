/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "change_offset.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "repaint.h"
#include "repair_damage.h"
#include "sbar.h"
#include "screen.h"

// Text has moved down by less than a screen, render lines that remain
static void copy_repaint_repair(const int16_t d, const int16_t y1,
	const int16_t y2, const int row1, const int row2)
{
	const uint16_t height = (jbxvt.scr.chars.height - d)
		* jbxvt.X.font_height;
#ifdef USE_XCB
	xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt, jbxvt.X.win.vt,
		jbxvt.X.gc.tx, 0, y1, 0, y2, jbxvt.scr.pixels.width,
		height);
#else//!USE_XCB
	XCopyArea(jbxvt.X.dpy, jbxvt.X.win.vt,
		jbxvt.X.win.vt, jbxvt.X.gc.tx, 0, y1,
		jbxvt.scr.pixels.width, height, 0, y2);
#endif//USE_XCB
	repaint((xcb_point_t){.y=row1}, (xcb_point_t){.y=row2,
		.x=jbxvt.scr.chars.width-1});
	repair_damage();
}

//  Change the value of the scrolled screen offset and repaint the screen
void change_offset(int16_t n)
{
	if (n > jbxvt.scr.sline.top)
		n = jbxvt.scr.sline.top;
	if (n < 0)
		n = 0;
	if (n == jbxvt.scr.offset)
		return;
	cursor(CURSOR_DRAW);
	int16_t d = n - jbxvt.scr.offset;
	jbxvt.scr.offset = n;
	if (d > 0 && d < jbxvt.scr.chars.height) {
		/*  Text has moved down by less than a screen so raster
		 *  the lines that did not move off.  */
		copy_repaint_repair(d, MARGIN,
			MARGIN + d * jbxvt.X.font_height,
			0, d - 1);
	} else if (d < 0 && -d < jbxvt.scr.chars.height) {
		/*  Text has moved down by less than a screen so raster
		 *  the lines that did not move off.  */
		d = -d;
		copy_repaint_repair(d, MARGIN + d * jbxvt.X.font_height,
			MARGIN, jbxvt.scr.chars.height - d,
			jbxvt.scr.chars.height - 1);
	} else
		  repaint((xcb_point_t){}, (xcb_point_t){
			  .y=jbxvt.scr.chars.height-1,
			  .x=jbxvt.scr.chars.width-1});
	cursor(CURSOR_DRAW);
	// Update current scrollbar position due to change
	sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
		jbxvt.scr.offset, jbxvt.scr.offset
		+ jbxvt.scr.chars.height - 1);
}

