/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "scr_erase.h"
#include "config.h"
#include "cursor.h"
#include "font.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"
#include "size.h"
#include "window.h"
#include <string.h>
#define DEBUG_ERASE
#ifndef DEBUG_ERASE
#undef LOG
#define LOG(...)
#endif//DEBUG_ERASE
static inline int16_t get_x(void)
{
	return jbxvt_get_screen()->cursor.x;
}
static inline int16_t get_y(void)
{
	return jbxvt_get_screen()->cursor.y;
}
static inline uint16_t get_width(void)
{
	return jbxvt_get_char_size().width;
}
static inline uint16_t get_height(void)
{
	return jbxvt_get_char_size().height;
}
static void del(xcb_connection_t * xc, uint16_t col, uint16_t width)
{
	struct JBXVTScreen * restrict s = jbxvt_get_screen();
	{ // cw scope
		const uint16_t cw = get_width();
		if (col + width > cw) // keep in screen
			width = cw - col;
	}
	const int16_t y = s->cursor.y;
	memset(s->text[y] + col, 0, width);
	memset(s->rend[y] + col, 0, width << 2);
	{ // f scope
		const struct JBDim f = jbxvt_get_font_size();
		xcb_clear_area(xc, 0, jbxvt_get_vt_window(xc), col * f.w,
			y * f.h, width * f.w, f.h);
	}
	xcb_flush(xc);
	s->wrap[y] = false;
	s->dwl[y] = false;
}
// Erase the specified portion of a line.
void jbxvt_erase_line(xcb_connection_t * xc, const int8_t mode)
{
	jbxvt_set_scroll(xc, 0);
	switch (mode) {
	case JBXVT_ERASE_ALL:
		del(xc, 0, get_width());
		break;
	case JBXVT_ERASE_BEFORE:
		del(xc, 0, get_x());
		break;
	case JBXVT_ERASE_AFTER:
	default: {
		const int16_t x = get_x();
		del(xc, x, get_width() - x);
	}
	}
	jbxvt_draw_cursor(xc);
}
struct ErasureRange {
	int16_t start, end;
};
static bool assign_range(xcb_connection_t * restrict xc,
	const int8_t mode, struct ErasureRange * restrict range)
{
	switch (mode) {
		// offset by 1 to not include current line, handled later
	case JBXVT_ERASE_AFTER:
		range->start = get_y() + 1;
		range->end = jbxvt_get_char_size().h;
		break;
	case JBXVT_ERASE_BEFORE:
		range->start = 0;
		range->end = get_y() - 1;
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
	struct ErasureRange range;
	if (!assign_range(xc, mode, &range))
		return;
	/* Save cursor y locally instead of using save/restore cursor
	   functions in order to avoid side-effects on applications
	   using a saved cursor position.  */
	{ // *y scope, old_y scope
		int16_t * y = &jbxvt_get_screen()->cursor.y;
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
