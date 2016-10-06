/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_erase.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"

#include <string.h>

//#define DEBUG_ERASE
#ifndef DEBUG_ERASE
#undef LOG
#define LOG(...)
#endif


static void zero(const uint16_t line, const size_t sz, uint16_t col)
{
	//col = MIN(col, jbxvt.scr.chars.width); // restrict bounds
	if (col > jbxvt.scr.chars.width) // outside screen area
		return; // do nothing
	if (col + sz > JBXVT_MAX_COLS)
		return; // don't overflow
	// check memory
	memset(jbxvt.scr.current->text[line] + col, 0, sz);
	memset(jbxvt.scr.current->rend[line] + col, 0, sz << 2);
	jbxvt.scr.current->wrap[line] = false;
}

#define CUR jbxvt.scr.current->cursor
#define FSZ jbxvt.X.font.size

static void erase_range(xcb_rectangle_t * restrict h,
	const size_t sz, const uint16_t col)
{
	h->x = col * FSZ.w;
	h->width = sz * FSZ.w;
	zero(CUR.y, sz, col);
}

static int16_t get_col(const uint8_t mode)
{
	return mode == 0 ? CUR.x : 0;
}

static int16_t get_sz(const uint8_t mode)
{
	fix_rc(&CUR);
	switch(mode) {
	case 0: // to end (cursor column to screen width)
		if (jbxvt.scr.chars.w < CUR.x) // invalid/negative range
			return 0; // erase nothing
		else // valid range
			return jbxvt.scr.chars.w - CUR.x;
	case 2: // entire (col 0 to screen width)
		return jbxvt.scr.chars.w;
	case 1: // from start (col 0 to current cursor column)
	default:
		return CUR.x;
	}
}

//  erase part or the whole of a line
void jbxvt_erase_line(const int8_t mode)
{
	LOG("jbxvt_erase_line(%d)", mode);
	jbxvt_set_scroll(0);
	struct JBDim c = jbxvt.scr.current->cursor;
	xcb_rectangle_t g = { .y = c.y * FSZ.h };
	erase_range(&g, get_sz(mode), get_col(mode));
	draw_cursor(); //clear
	jbxvt_check_selection(c.y, c.y);
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt,
		g.x, g.y, g.width, FSZ.h);
	jbxvt.scr.current->wrap_next = false;
	draw_cursor();
	xcb_flush(jbxvt.X.xcb);
}

//  erase part or the whole of the screen
void jbxvt_erase_screen(const int8_t mode)
{
	LOG("jbxvt_erase_screen(mode=%d)", mode);
	uint16_t start, end;
	switch (mode) {
		// offset by 1 to not include current line, handled later
	case 0: // below
		start = jbxvt.scr.current->cursor.y + 1;
		end = jbxvt.scr.chars.h;
		break;
	case 1: // above
		start = 0;
		end = jbxvt.scr.current->cursor.y - 1;
		break;
	case 3: // saved lines
		jbxvt_clear_saved_lines();
		return;
	default: // all
		start = 0;
		end = jbxvt.scr.chars.h - 1;
		break;
	}
	save_cursor();
	for (uint_fast16_t l = start; l < end; ++l) {
		jbxvt.scr.current->cursor.y = l;
		jbxvt_erase_line(2); // entire
	}
	restore_cursor();
	// clear start of, end of, or entire current line, per mode
	jbxvt_erase_line(mode);
}

