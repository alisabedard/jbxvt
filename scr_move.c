// Copyright 2016, Jeffrey E. Bedard
#include "scr_move.h"
#include "cursor.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "mode.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "size.h"
#ifndef JBXVT_SCR_MOVE_DEBUG
#undef LOG
#define LOG(...)
#endif//!JBXVT_SCR_MOVE_DEBUG
// Ensure cursor coordinates are valid per screen and decom mode
// Returns new cursor y value
int16_t jbxvt_check_cursor_position(void)
{
	struct JBXVTScreen * restrict s = jbxvt_get_screen();
	struct JBDim * restrict c = &s->cursor;
	struct JBDim * restrict m = &s->margin;
	struct JBDim sz = jbxvt_get_char_size();
	--sz.w; --sz.h;
	m->top = JB_MAX(m->top, 0); // Sanitize margins
	m->bottom = JB_MIN(m->bottom, sz.h);
	if (jbxvt_get_modes()->decom) // Implement DECOM
		JB_LIMIT(c->y, m->top, m->bottom);
	JB_LIMIT(c->x, sz.w, 0);
	JB_LIMIT(c->y, sz.h, 0);
	return c->y;
}
static inline int16_t dim(const int16_t cursor,
	const int16_t delta, const bool relative)
{
	// Sanitize non-relative arguments--must be positive.
	return relative ? cursor + delta : JB_MAX(delta, 0);
}
/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void jbxvt_move(xcb_connection_t * xc,
	const int16_t x, const int16_t y, const uint8_t relative)
{
	LOG("jbxvt_move(x:%d, y:%d, relative:%d)", x, y, relative);
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc); // clear
	struct JBXVTScreen * restrict s = jbxvt_get_screen();
	struct JBDim c = s->cursor;
	s->cursor = c = (struct JBDim) { .x = dim(c.x, x,
		relative & JBXVT_COLUMN_RELATIVE), .y = dim(c.y, y,
			relative & JBXVT_ROW_RELAATIVE)};
	c.y = jbxvt_check_cursor_position();
	s->wrap_next = false;
	jbxvt_check_selection(xc, c.y, c.y);
	jbxvt_draw_cursor(xc); // draw
}
