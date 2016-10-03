// Copyright 2016, Jeffrey E. Bedard

#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "sbar.h"

// Sanitize cursor position, implement DECOM
void reset_row_col(void)
{
	struct JBDim * c = &SCR->cursor;
	JB_LIMIT(c->x, CSZ.w - 1, 0);
	JB_LIMIT(c->y, CSZ.h - 1, 0);
	// Implement DECOM, DEC Origin Mode, limits
	if (jbxvt.mode.decom)
		JB_LIMIT(c->y, SCR->margin.t, SCR->margin.b);
}

static void set_dimension(int16_t * restrict cursor,
	const int16_t delta, const bool relative)
{
	*cursor = relative ? *cursor + delta : MAX(delta, 0);
}

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void scr_move(const int16_t x, const int16_t y, const uint8_t relative)
{
#ifdef MOVE_DEBUG
	LOG("scr_move(x:%d, y:%d, relative:%d)", x, y, relative);
#endif//MOVE_DEBUG
	change_offset(0);
	draw_cursor(); // clear
	struct JBDim * c = &SCR->cursor;
	// Sanitize non-relative arguments--must be positive.
	set_dimension(&c->x, x, relative & COL_RELATIVE);
	set_dimension(&c->y, y, relative & ROW_RELATIVE);
	reset_row_col();
	const int16_t cy = c->y;
	SCR->wrap_next = 0;
	check_selection(cy, cy);
	draw_cursor(); // draw
}

