/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "cursor.h"
#include "config.h"
#include "font.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "mode.h"
#include "repaint.h"
#include "rstyle.h"
#include "sbar.h"
#include "screen.h"
#include "size.h"
#include "window.h"
static uint32_t saved_style;
static struct JBDim saved_cursor;
static uint8_t cursor_attr = 1;
void jbxvt_blink_cursor(xcb_connection_t * xc)
{
	if (!jbxvt_get_modes()->att610 && cursor_attr % 2) {
		jbxvt_draw_cursor(xc); // blinking cursor
		xcb_flush(xc);
	}
}
void jbxvt_set_cursor_attr(const uint8_t val)
{
	cursor_attr = val;
}
xcb_gcontext_t jbxvt_get_cursor_gc(xcb_connection_t * xc)
{
	static xcb_gcontext_t gc;
	if (gc)
		return gc;
	return gc = xcb_generate_id(xc);
}
void jbxvt_save_cursor(void)
{
	saved_cursor = jbxvt_get_screen()->cursor;
	saved_style = jbxvt_get_rstyle();
}
void jbxvt_restore_cursor(xcb_connection_t * xc)
{
	jbxvt_draw_cursor(xc);
	jbxvt_get_screen()->cursor = saved_cursor;
	jbxvt_zero_rstyle();
	jbxvt_add_rstyle(saved_style);
	jbxvt_draw_cursor(xc);
}
static bool is_blinking(void)
{
	switch (cursor_attr) {
	case 0: // blinking block
	case 1: // blinking block
	case 3: // blinking underline
	case 5: // blinking bar
	case 7: // blinking overline
		return true;
	}
	return false;
}
void jbxvt_draw_cursor(xcb_connection_t * xc)
{
	// Don't draw if scrolled, non-existent, or hidden
	struct JBXVTScreen * current;
	if (jbxvt_get_scroll() || !(current = jbxvt_get_screen())
		|| !jbxvt_get_modes()->dectcem)
		return;
	if ((current->cursor_visible ^= true) && is_blinking())
		jbxvt_repaint(xc); // prevent stale cursor blocks
	struct JBDim p = jbxvt_chars_to_pixels(current->cursor);
	const struct JBDim f = jbxvt_get_font_size();
	xcb_rectangle_t r = {p.x, p.y, f.w, f.h};
	switch (cursor_attr) {
	case 0: // blinking block
	case 1: // blinking block
	case 2: // steady block (default)
		break;
	case 3: // blinking underline
	case 4: // steady underline
		r.height = 2;
		r.y += f.h - 2;
		break;
	case 5: // blinking bar
	case 6: // steady bar
		r.width = 2;
		break;
	case 7: // blinking overline
	case 8: // steady overline
		r.height = 2;
		break;
	}
	xcb_poly_fill_rectangle(xc, jbxvt_get_vt_window(xc),
		jbxvt_get_cursor_gc(xc), 1, &r);
}
