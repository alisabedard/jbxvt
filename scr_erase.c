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

static void zero_line(uint8_t * restrict s,
	uint32_t * restrict r, uint16_t sz)
{
	++sz;
	memset(s, 0, sz);
	memset(r, 0, sz * sizeof(uint32_t));
}

static void get_horz_geo(XRectangle * restrict h,
	const uint16_t sz, const uint16_t col)
{
	h->x = MARGIN + col * jbxvt.X.font_width;
	h->width = sz * jbxvt.X.font_width;
}

//  erase part or the whole of a line
void scr_erase_line(const int8_t mode)
{
	home_screen();
	XRectangle g = { .y = MARGIN + jbxvt.scr.current->cursor.row
			* jbxvt.X.font_height
	};
	uint8_t * s = jbxvt.scr.current->text[jbxvt.scr.current->cursor.row];
	uint32_t * r = jbxvt.scr.current->rend[jbxvt.scr.current->cursor.row];
	switch (mode) {
	    case START :
		get_horz_geo(&g, jbxvt.scr.current->cursor.col, 0);
		zero_line(s, r, jbxvt.scr.current->cursor.col);
		break;
	    case END :
		get_horz_geo(&g, jbxvt.scr.chars.width
			- jbxvt.scr.current->cursor.col,
			jbxvt.scr.current->cursor.col);
		zero_line(s + jbxvt.scr.current->cursor.col,
			r + jbxvt.scr.current->cursor.col,
			jbxvt.scr.chars.width
			- jbxvt.scr.current->cursor.col);
		break;
	    case ENTIRE :
		get_horz_geo(&g, jbxvt.scr.chars.width, 0);
		zero_line(s, r, jbxvt.scr.chars.width);
		break;
	    default :
		return;
	}
	/*  patch in the final rendition flag if there is any non-zero
	 *  rendition.  */
	r[jbxvt.scr.chars.width] = 0;
	for (uint16_t i = 0; i < jbxvt.scr.chars.width; i++) {
		if (r[i] != 0) {
			r[jbxvt.scr.chars.width] = 1;
			break;
		}
	}
	cursor(CURSOR_DRAW); //clear
	check_selection(jbxvt.scr.current->cursor.row,
		jbxvt.scr.current->cursor.row);
#ifdef USE_XCB
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, g.x, g.y, g.width,
		jbxvt.X.font_height);
#else//!USE_XCB
	XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt, g.x, g.y,
		g.width, jbxvt.X.font_height, false);
#endif//USE_XCB
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

//  erase part or the whole of the screen
void scr_erase_screen(const int8_t mode)
{
	home_screen();
	jbxvt.scr.current->wrap_next = 0;
	uint16_t i, width = jbxvt.X.font_width * jbxvt.scr.chars.width;
	const uint16_t wsz = jbxvt.scr.chars.width + 1;
	int16_t x = MARGIN, y, height;
	switch (mode) {
	    case START :
		y = MARGIN;
		height = jbxvt.scr.current->cursor.row * jbxvt.X.font_height;
		for (i = 0; i < jbxvt.scr.current->cursor.row; i++) {
			memset(jbxvt.scr.current->text[i],0, wsz);
			memset(jbxvt.scr.current->rend[i],0,
				wsz * sizeof(int32_t));
		}
		check_selection(0,jbxvt.scr.current->cursor.row - 1);
		if (height > 0)
			XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
				x,y,width,height,False);
		scr_erase_line(mode);
		break;
	    case END :
		if (jbxvt.scr.current->cursor.row
			|| jbxvt.scr.current->cursor.col) {
			y = MARGIN + (jbxvt.scr.current->cursor.row + 1)
				* jbxvt.X.font_height;
			height = (jbxvt.scr.chars.height
				- jbxvt.scr.current->cursor.row - 1)
				* jbxvt.X.font_height;
			for (i = jbxvt.scr.current->cursor.row + 1;
				i < jbxvt.scr.chars.height; i++) {
				memset(jbxvt.scr.current->text[i],0, wsz);
				memset(jbxvt.scr.current->rend[i],0,
					wsz * sizeof(uint32_t));
			}
			check_selection(jbxvt.scr.current->cursor.row + 1,
				jbxvt.scr.chars.height - 1);
			if (height > 0) {
#ifdef USE_XCB
				xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
					x, y, width, height);
#else//!USE_XCB
				XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
					x,y,width,height,False);
#endif//USE_XCB
			}
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
		cursor(CURSOR_DRAW);
		check_selection(0,jbxvt.scr.chars.height - 1);
#ifdef USE_XCB
		xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, x, y,
			width, height);
#else//!USE_XCB
		XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
			x,y,width,height,False);
#endif//USE_XCB
		cursor(CURSOR_DRAW);
		sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
			0, jbxvt.scr.chars.height - 1);
		break;
	    default :
		return;
	}
}

