/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_erase.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "sbar.h"
#include "screen.h"
#include "scroll_up.h"
#include "selection.h"

#include <string.h>

static int16_t zero_line(uint8_t * restrict str,
	uint32_t * restrict rend, uint16_t * restrict width,
	uint8_t sz, const uint8_t col)
{
	sz++;
	fprintf(stderr, "%d, %d\n", sz, col);
	memset(str, 0, sz);
	memset(rend, 0, sz * sizeof(uint32_t));
	*width = sz * jbxvt.X.font_width;
	return MARGIN + col * jbxvt.X.font_width;
}

//  erase part or the whole of a line
static void scr_erase_line(int mode)
{
	home_screen();
	XRectangle g;
	g.y = MARGIN + jbxvt.scr.current->row * jbxvt.X.font_height;
	g.height = jbxvt.X.font_height;
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->row];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->row];
	size_t sz;
	switch (mode) {
	    case START:
		    sz = jbxvt.scr.current->col;
		    g.x = zero_line(s, r, &g.width, sz, 0);
		    break;
	    case END:
		    sz = jbxvt.scr.chars.width - jbxvt.scr.current->col;
		    g.x = zero_line(s, r, &g.width, sz, jbxvt.scr.current->col);
		    g.width+=jbxvt.X.font_width;
		    break;
	    case ENTIRE:
		    sz = jbxvt.scr.chars.width;
		    g.x = zero_line(s, r, &g.width, sz, 0);
		    break;
	    default:
		    return;
	}
	/*  patch in the final rendition flag if there is any non-zero
	 *  rendition.
	 */
	r[jbxvt.scr.chars.width] = 0;
	for (uint8_t i = 0; i < jbxvt.scr.chars.width; i++) {
		if (r[i] != 0) {
			r[jbxvt.scr.chars.width] = 1;
			break;
		}
	}
	cursor(); //clear
	check_selection(jbxvt.scr.current->row, jbxvt.scr.current->row);
	XClearArea(jbxvt.X.dpy, jbxvt.X.win.vt, g.x, g.y,
		g.width, g.height, False);
	jbxvt.scr.current->wrap_next = 0;
	cursor();
}

//  erase part or the whole of the screen
void scr_erase_screen(int mode)
{
	home_screen();
	jbxvt.scr.current->wrap_next = 0;
	uint16_t i, width = jbxvt.X.font_width * jbxvt.scr.chars.width;
	const uint16_t wsz = jbxvt.scr.chars.width + 1;
	int16_t x = MARGIN, y, height;
	switch (mode) {
	    case START :
		y = MARGIN;
		height = jbxvt.scr.current->row * jbxvt.X.font_height;
		for (i = 0; i < jbxvt.scr.current->row; i++) {
			memset(jbxvt.scr.current->text[i],0, wsz);
			memset(jbxvt.scr.current->rend[i],0,
				wsz * sizeof(int32_t));
		}
		check_selection(0,jbxvt.scr.current->row - 1);
		if (height > 0)
			XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
				x,y,width,height,False);
		scr_erase_line(mode);
		break;
	    case END :
		if (jbxvt.scr.current->row || jbxvt.scr.current->col) {
			y = MARGIN + (jbxvt.scr.current->row + 1)
				* jbxvt.X.font_height;
			height = (jbxvt.scr.chars.height
				- jbxvt.scr.current->row - 1)
				* jbxvt.X.font_height;
			for (i = jbxvt.scr.current->row + 1;
				i < jbxvt.scr.chars.height; i++) {
				memset(jbxvt.scr.current->text[i],0, wsz);
				memset(jbxvt.scr.current->rend[i],0,
					wsz * sizeof(uint32_t));
			}
			check_selection(jbxvt.scr.current->row + 1,
				jbxvt.scr.chars.height - 1);
			if (height > 0)
				XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
					x,y,width,height,False);
			scr_erase_line(mode);
			break;
		}
		/*  If we are positioned at the top left hand corner then
		 *  it is effectively a whole screen clear.
		 *  Drop through so that we do not need to duplicate
		 *  the scroll-up code.  */
	    case ENTIRE :
		y = MARGIN;
		height = jbxvt.scr.chars.height * jbxvt.X.font_height;
		if (jbxvt.scr.current == &jbxvt.scr.s1)
			scroll_up(jbxvt.scr.chars.height);
		else
			for (i = 0; i < jbxvt.scr.chars.height; i++) {
				memset(jbxvt.scr.current->text[i],0, wsz);
				memset(jbxvt.scr.current->rend[i],0,
					wsz * sizeof(uint32_t));
			}
		cursor();
		check_selection(0,jbxvt.scr.chars.height - 1);
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
			x,y,width,height,False);
		cursor();
		sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
			0, jbxvt.scr.chars.height - 1);
		break;
	    default :
		return;
	}
}

