/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cursor.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "screen.h"

static uint32_t saved_style;
static xcb_point_t saved_cursor;

void save_cursor(void)
{
	struct JBXVTScreenData * s = &jbxvt.scr;
	saved_cursor = s->current->cursor;
	saved_style = s->rstyle;
}

void restore_cursor(void)
{
	draw_cursor();
	struct JBXVTScreenData * s = &jbxvt.scr;
	s->current->cursor = saved_cursor;
	s->rstyle = saved_style;
	draw_cursor();
}

static xcb_point_t get_p(xcb_point_t p)
{
	p.x *= jbxvt.X.f.size.width;
	p.y *= jbxvt.X.f.size.height;
	p.x += MARGIN;
	p.y += MARGIN;
	return p;
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

void draw_cursor(void)
{
	struct JBXVTScreenData * s = &jbxvt.scr;
	// Don't draw if scrolled, non-existent, or hidden
	VTScreen * current;
	if (s->offset || !(current = s->current) || !jbxvt.mode.dectcem)
		return;
	if ((current->cursor_visible ^= true) && is_blinking())
		repaint(); // prevent stale cursor blocks
	xcb_point_t p = get_p(s->current->cursor);
	struct JBXVTXData * X = &jbxvt.X;
	const Size f = X->f.size;
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
	xcb_poly_fill_rectangle(X->xcb, X->win.vt, X->gc.cu, 1, &r);
	xcb_flush(X->xcb); // Apply drawing
}

