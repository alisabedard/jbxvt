/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "cursor.h"
#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "screen.h"
#include "window.h"
static uint32_t saved_style;
static struct JBDim saved_cursor;
xcb_gcontext_t jbxvt_get_cursor_gc(xcb_connection_t * xc)
{
	static xcb_gcontext_t gc;
	if (gc)
		return gc;
	return gc = xcb_generate_id(xc);
}
void jbxvt_save_cursor(void)
{
	struct JBXVTScreenData * s = &jbxvt.scr;
	saved_cursor = s->current->cursor;
	saved_style = s->rstyle;
}
void jbxvt_restore_cursor(xcb_connection_t * xc)
{
	jbxvt_draw_cursor(xc);
	struct JBXVTScreenData * s = &jbxvt.scr;
	s->current->cursor = saved_cursor;
	s->rstyle = saved_style;
	jbxvt_draw_cursor(xc);
}
static bool is_blinking(void)
{
	switch (jbxvt.opt.cursor_attr) {
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
	struct JBXVTScreenData * s = &jbxvt.scr;
	// Don't draw if scrolled, non-existent, or hidden
	struct JBXVTScreen * current;
	if (s->offset || !(current = s->current) || !jbxvt.mode.dectcem)
		return;
	if ((current->cursor_visible ^= true) && is_blinking())
		jbxvt_repaint(xc); // prevent stale cursor blocks
	struct JBDim p = jbxvt_get_pixel_size(s->current->cursor);
	struct JBXVTXData * X = &jbxvt.X;
	const struct JBDim f = X->font.size;
	xcb_rectangle_t r = {p.x, p.y, f.w, f.h};
	switch (jbxvt.opt.cursor_attr) {
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
