// Copyright 2016, Jeffrey E. Bedard

#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "sbar.h"

// Sanitize cursor position, implement DECOM
void reset_row_col(void)
{
	struct JBDim * c = &jbxvt.scr.current->cursor;
	JB_LIMIT(c->x, jbxvt.scr.chars.w - 1, 0);
	JB_LIMIT(c->y, jbxvt.scr.chars.h - 1, 0);
	// Implement DECOM, DEC Origin Mode, limits
	if (jbxvt.mode.decom)
		JB_LIMIT(c->y, jbxvt.scr.current->margin.t, jbxvt.scr.current->margin.b);
}

static void set_dimension(int16_t * restrict cursor,
	const int16_t delta, const bool relative)
{
	*cursor = relative ? *cursor + delta : MAX(delta, 0);
}

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void jbxvt_move(const int16_t x, const int16_t y, const uint8_t relative)
{
#ifdef MOVE_DEBUG
	LOG("jbxvt_move(x:%d, y:%d, relative:%d)", x, y, relative);
#endif//MOVE_DEBUG
	jbxvt_set_scroll(0);
	draw_cursor(); // clear
	struct JBDim * c = &jbxvt.scr.current->cursor;
	// Sanitize non-relative arguments--must be positive.
	set_dimension(&c->x, x, relative & COL_RELATIVE);
	set_dimension(&c->y, y, relative & ROW_RELATIVE);
	reset_row_col();
	const int16_t cy = c->y;
	jbxvt.scr.current->wrap_next = 0;
	jbxvt_check_selection(cy, cy);
	draw_cursor(); // draw
}

