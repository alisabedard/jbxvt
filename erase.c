/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
//#undef DEBUG
#include "erase.h"
#include <string.h>
#include "cursor.h"
#include "font.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "size.h"
#include "window.h"
static inline uint16_t get_width(void)
{
	return jbxvt_get_char_size().width;
}
static inline uint16_t get_height(void)
{
	return jbxvt_get_char_size().height;
}
static void clear_area(xcb_connection_t * restrict xc,
	const int16_t x, const int16_t y, const uint16_t width)
{
	const struct JBDim f = jbxvt_get_font_size();
	xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), x * f.w, y * f.h,
		width * f.w, f.h);
}
static uint16_t get_limited_width(const uint16_t col, uint16_t width)
{
	const uint16_t cw = get_width();
	if (col + width > cw) // keep in screen
		width = cw - col;
	return width;
}
static void delete(xcb_connection_t * restrict xc, uint16_t col,
	uint16_t width)
{
	width = get_limited_width(col, width);
	struct JBXVTScreen * restrict s = jbxvt_get_current_screen();
	const int16_t y = s->cursor.y;
	struct JBXVTLine * restrict l = s->line + y;
	memset(l->text + col, 0, width);
	memset(l->rend + col, 0, width << 2);
	clear_area(xc, col, y, width);
	l->wrap = l->dwl = false;
}
static inline void delete_after(xcb_connection_t * restrict xc,
	const int16_t x)
{
	delete(xc, x, get_width() - x);
}
// Erase the specified portion of a line.
void jbxvt_erase_line(xcb_connection_t * xc, const int8_t mode)
{
	LOG("jbxvt_erase_line(xc, mode: %d)", mode);
	jbxvt_set_scroll(xc, 0);
	switch (mode) {
	case JBXVT_ERASE_ALL:
		delete_after(xc, 0);
		break;
	case JBXVT_ERASE_BEFORE:
		delete(xc, 0, jbxvt_get_x());
		break;
	case JBXVT_ERASE_AFTER:
	default:
		delete_after(xc, jbxvt_get_x());
	}
	jbxvt_draw_cursor(xc);
}
static bool assign_range(xcb_connection_t * restrict xc,
	const int8_t mode, struct JBDim * restrict range)
{
	switch (mode) {
		// offset by 1 to not include current line, handled later
	case JBXVT_ERASE_AFTER:
		range->start = jbxvt_get_y() + 1;
		range->end = jbxvt_get_char_size().h;
		break;
	case JBXVT_ERASE_BEFORE:
		range->start = 0;
		range->end = jbxvt_get_y() - 1;
		break;
	case JBXVT_ERASE_SAVED:
		jbxvt_clear_saved_lines(xc);
		return false;
	default: // JBXVT_ERASE_ALL
		range->start = 0;
		range->end = get_height() - 1;
		break;
	}
	return true;
}
// Erase the specified portion of the screen.
void jbxvt_erase_screen(xcb_connection_t * xc, const int8_t mode)
{
	LOG("jbxvt_erase_screen(mode=%d)", mode);
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
