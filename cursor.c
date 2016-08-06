/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cursor.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
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
	struct JBXVTScreenData * s = &jbxvt.scr;
	s->current->cursor = saved_cursor;
	s->rstyle = saved_style;
}

static xcb_point_t get_p(void)
{
	xcb_point_t p = jbxvt.scr.current->cursor;
	p.x *= jbxvt.X.f.size.width;
	p.y *= jbxvt.X.f.size.height;
	p.x += MARGIN;
	p.y += MARGIN;
	return p;
}

void draw_cursor(void)
{
	// Don't draw cursor when scrolled
	if (jbxvt.scr.offset > 0)
		return;
	if (!jbxvt.scr.current)
		return; // prevent segfault
	if (!jbxvt.mode.dectcem) // hide cursor
		return;
	xcb_point_t p = get_p();
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

