/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cursor.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "screen.h"

static uint32_t saved_style;
static struct JBDim saved_cursor;

void jbxvt_save_cursor(void)
{
	struct JBXVTScreenData * s = &jbxvt.scr;
	saved_cursor = s->current->cursor;
	saved_style = s->rstyle;
}

void jbxvt_restore_cursor(void)
{
	jbxvt_draw_cursor();
	struct JBXVTScreenData * s = &jbxvt.scr;
	s->current->cursor = saved_cursor;
	s->rstyle = saved_style;
	jbxvt_draw_cursor();
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

void jbxvt_draw_cursor(void)
{
	struct JBXVTScreenData * s = &jbxvt.scr;
	// Don't draw if scrolled, non-existent, or hidden
	struct JBXVTScreen * current;
	if (s->offset || !(current = s->current) || !jbxvt.mode.dectcem)
		return;
	if ((current->cursor_visible ^= true) && is_blinking())
		jbxvt_repaint(); // prevent stale cursor blocks
	struct JBDim p = jbxvt_get_pixel_size(s->current->cursor);
	struct JBXVTXData * X = &jbxvt.X;
	const struct JBDim f = X->f.size;
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
}

