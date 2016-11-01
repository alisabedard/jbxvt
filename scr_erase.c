/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "scr_erase.h"
#include "config.h"
#include "cursor.h"
#include "font.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"
#include "window.h"
#include <string.h>
#define DEBUG_ERASE
#ifndef DEBUG_ERASE
#undef LOG
#define LOG(...)
#endif//DEBUG_ERASE
static void del(xcb_connection_t * xc, uint16_t col, uint16_t width)
{
	const uint16_t y = jbxvt.scr.current->cursor.y;
	struct JBXVTScreen * s = jbxvt.scr.current;
	if (col + width > jbxvt.scr.chars.width) // keep in screen
		width = jbxvt.scr.chars.width - col;
	memset(s->text[y] + col, 0, width);
	memset(s->rend[y] + col, 0, width << 2);
	const struct JBDim f = jbxvt_get_font_size();
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), col * f.w,
		y * f.h, width * f.w, f.h);
	xcb_flush(xc);
	s->wrap[y] = false;
	s->dwl[y] = false;
}
//  erase part or the whole of a line
void jbxvt_erase_line(xcb_connection_t * xc, const int8_t mode)
{
	jbxvt_set_scroll(xc, 0);
	const uint16_t x = jbxvt.scr.current->cursor.x;
	switch (mode) {
	case JBXVT_ERASE_ALL:
		del(xc, 0, jbxvt.scr.chars.width);
		break;
	case JBXVT_ERASE_BEFORE:
		del(xc, 0, x);
		break;
	case JBXVT_ERASE_AFTER:
	default:
		del(xc, x, jbxvt.scr.chars.width - x);
	}
	jbxvt_draw_cursor(xc);
}
//  erase part or the whole of the screen
void jbxvt_erase_screen(xcb_connection_t * xc, const int8_t mode)
{
	LOG("jbxvt_erase_screen(mode=%d)", mode);
	uint16_t start, end;
	switch (mode) {
		// offset by 1 to not include current line, handled later
	case JBXVT_ERASE_AFTER:
		start = jbxvt.scr.current->cursor.y + 1;
		end = jbxvt.scr.chars.h;
		break;
	case JBXVT_ERASE_BEFORE:
		start = 0;
		end = jbxvt.scr.current->cursor.y - 1;
		break;
	case JBXVT_ERASE_SAVED:
		jbxvt_clear_saved_lines(xc);
		return;
	default: // JBXVT_ERASE_ALL
		start = 0;
		end = jbxvt.scr.chars.h - 1;
		break;
	}
	/* Save cursor y locally instead of using save/restore cursor
	   functions in order to avoid side-effects on applications
	   using a saved cursor position.  */
	const uint16_t old_y = jbxvt.scr.current->cursor.y;
	for (uint_fast16_t l = start; l <= end; ++l) {
		jbxvt.scr.current->cursor.y = l;
		jbxvt_erase_line(xc, JBXVT_ERASE_ALL); // entire
		jbxvt_draw_cursor(xc);
	}
	jbxvt.scr.current->cursor.y = old_y;
	// clear start of, end of, or entire current line, per mode
	jbxvt_erase_line(xc, mode);
}
