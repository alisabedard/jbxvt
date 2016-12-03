/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "edit.h"
#include "cursor.h"
#include "font.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "paint.h"
#include "screen.h"
#include "size.h"
#include "window.h"
#define DEBUG_EDIT
#ifndef DEBUG_EDIT
#undef LOG
#define LOG(...)
#endif//!DEBUG_EDIT
static inline uint16_t get_copy_width(const uint8_t count)
{
	const uint16_t w = jbxvt_get_char_size().w - count - jbxvt_get_x();
	return w * jbxvt_get_font_size().width;
}
static void copy_area(xcb_connection_t * restrict xc,
	const int16_t * restrict x, const int16_t y,
	const uint8_t count)
{
	const xcb_window_t v = jbxvt_get_vt_window(xc);
	xcb_copy_area(xc, v, v, jbxvt_get_text_gc(xc), x[0], y,
		x[1], y, get_copy_width(count),
		jbxvt_get_font_size().height);
}
void jbxvt_edit_characters(xcb_connection_t * xc, const uint8_t count,
	const bool delete)
{
	LOG("jbxvt_edit_characters(count: %d, delete: %s)", count,
		delete ? "true" : "false");
	const struct JBDim c = jbxvt_chars_to_pixels(jbxvt_get_cursor());
	int16_t x[2] = {c.x, x[0] + count * jbxvt_get_font_size().width};
	/* Whether or not x[0] and x[1] are swapped here is
	   what determines insertion or deletion.  */
	if (delete)
		JB_SWAP(int16_t, x[0], x[1]);
	jbxvt_draw_cursor(xc);
	copy_area(xc, x, c.y, count);
	jbxvt_get_current_screen()->wrap_next = 0;
	jbxvt_draw_cursor(xc);

}
