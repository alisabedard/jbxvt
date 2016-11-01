// Copyright 2016, Jeffrey E. Bedard
#include "scr_move.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "sbar.h"
#ifndef JBXVT_SCR_MOVE_DEBUG
#undef LOG
#define LOG(...)
#endif//!JBXVT_SCR_MOVE_DEBUG
// Sanitize cursor position, implement DECOM
static int16_t decom(struct JBDim * restrict c)
{
	// Implement DECOM, DEC Origin Mode, limits
	if (jbxvt.mode.decom) {
		const struct JBDim m = jbxvt.scr.current->margin;
		JB_LIMIT(c->y, m.top, m.bottom);
	}
	return c->y;
}
// Ensure cursor coordinates are valid per screen and decom mode
// Returns new cursor y value
int16_t jbxvt_check_cursor_position(void)
{
	struct JBDim * c = &jbxvt.scr.current->cursor;
	JB_LIMIT(c->x, jbxvt.scr.chars.w - 1, 0);
	JB_LIMIT(c->y, jbxvt.scr.chars.h - 1, 0);
	return decom(c);
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
	struct JBDim c = jbxvt.scr.current->cursor;
	jbxvt.scr.current->cursor = c
		= (struct JBDim) { .x = dim(c.x, x, relative
			& JBXVT_COLUMN_RELATIVE),
		.y = dim(c.y, y, relative & JBXVT_ROW_RELAATIVE)};
	c.y = jbxvt_check_cursor_position();
	jbxvt.scr.current->wrap_next = false;
	jbxvt_check_selection(xc, c.y, c.y);
	jbxvt_draw_cursor(xc); // draw
}
