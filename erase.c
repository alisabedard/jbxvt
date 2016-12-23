/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#undef DEBUG
#include "erase.h"
#include <stdbool.h>
#include <string.h>
#include <xcb/xproto.h>
#include "JBXVTLine.h"
#include "JBXVTScreen.h"
#include "config.h"
#include "cursor.h"
#include "font.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "paint.h"
#include "rstyle.h"
#include "sbar.h"
#include "scr_move.h"
#include "screen.h"
#include "scroll.h"
#include "size.h"
#include "string.h"
#include "window.h"
static void clear_area(xcb_connection_t * restrict xc,
	const int16_t x, const int16_t y, const uint16_t width)
{
	const struct JBDim f = jbxvt_get_font_size();
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), x * f.w, y * f.h,
		width * f.w, f.h);
}
static uint16_t get_limited_width(const uint16_t col, const uint16_t width)
{
	const int cw = jbxvt_get_char_size().width;
	// keep col + width within the screen width
	return (col + width > cw) ? cw - col : width;
}
static void delete(xcb_connection_t * restrict xc, const uint16_t col,
	uint16_t width)
{
	const int16_t y = jbxvt_get_y();
	LOG("\tdelete(xc, row: %d, col:%d, width: %d)", y, col, width);
	width = get_limited_width(col, width);
	struct JBXVTLine * l = jbxvt_get_line(y);
	memset(l->text + col, 0, width);
	memset(l->rend + col, 0, width << 2);
	clear_area(xc, col, y, width);
	l->wrap = l->dwl = false;
}
#ifdef DEBUG
static char * debug_mode(const int8_t mode)
{
	switch(mode) {
	case 0:
		return "0:After";
	case 1:
		return "1:Before";
	case 2:
		return "2:All";
	case 3:
		return "3:Saved";
	default:
		return "?:Unknown";
	}
}
#endif//DEBUG
static void jbxvt_erase_after(xcb_connection_t * xc)
{
	jbxvt_check_cursor_position();
	const int16_t y = jbxvt_get_y();
	struct JBXVTLine * l = jbxvt_get_line(y);
	uint8_t * t = l->text;
	const int16_t x = jbxvt_get_x();
	delete(xc, x, jbxvt_get_char_size().width - x);
	t[x] = 0;
	l->size = x;
	LOG("(row: %d col: %d) %s", y, x, t);
}

// Erase the specified portion of a line.
void jbxvt_erase_line(xcb_connection_t * xc, const int8_t mode)
{
	LOG("jbxvt_erase_line(xc, mode: %s)", debug_mode(mode));
	jbxvt_set_scroll(xc, 0);
	switch (mode) {
	case JBXVT_ERASE_ALL:
		delete(xc, 0, jbxvt_get_char_size().width);
		break;
	case JBXVT_ERASE_BEFORE:
		delete(xc, 0, jbxvt_get_x());
		break;
	case JBXVT_ERASE_AFTER:
	default:
		jbxvt_erase_after(xc);
	}
	jbxvt_draw_cursor(xc);
}
static bool assign_range(xcb_connection_t * restrict xc,
	const int8_t mode, struct JBDim * restrict range)
{
#define SETR(s, e) range->start = s; range->end = e;
	switch (mode) {
		// offset by 1 to not include current line, handled later
	case JBXVT_ERASE_AFTER:
		SETR(jbxvt_get_y() + 1, jbxvt_get_char_size().h);
		break;
	case JBXVT_ERASE_BEFORE:
		SETR(0, jbxvt_get_y() - 1);
		break;
	case JBXVT_ERASE_SAVED:
		jbxvt_clear_saved_lines(xc);
		return false;
	default: // JBXVT_ERASE_ALL
		SETR(0, jbxvt_get_char_size().height - 1);
		break;
	}
	return true;
#undef SETR
}
// Erase the specified portion of the screen.
void jbxvt_erase_screen(xcb_connection_t * xc, const int8_t mode)
{
	LOG("jbxvt_erase_screen(mode=%s)", debug_mode(mode));
	struct JBDim range;
	if (!assign_range(xc, mode, &range))
		return;
	/* Save cursor y locally instead of using save/restore cursor
	   functions in order to avoid side-effects on applications
	   using a saved cursor position.  */
	{ // *y scope, old_y scope
		int16_t * y = &jbxvt_get_current_screen()->cursor.y;
		const int16_t old_y = *y;
		for (int16_t l = range.start; l <= range.end; ++l) {
			*y = l;
			jbxvt_erase_line(xc, JBXVT_ERASE_ALL); // entire
			jbxvt_draw_cursor(xc);
		}
		*y = old_y;
	}
	// clear start of, end of, or entire current line, per mode
	jbxvt_erase_line(xc, mode);
}
