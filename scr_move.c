#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"

#include <stdlib.h>

// Sanitize cursor position, implement DECOM
void reset_row_col(void)
{
	xcb_point_t * c = &jbxvt.scr.current->cursor;
#define CH jbxvt.scr.chars
#define LIMIT(var, top, bottom) var = MAX(MIN(var, top), bottom)
	LIMIT(c->x, CH.w - 1, 0);
	LIMIT(c->y, CH.h - 1, 0);
	// Implement DECOM, DEC Origin Mode, limits
	if (jbxvt.mode.decom)
#define M jbxvt.scr.current->margin
		LIMIT(c->y, M.t, M.b);
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
	xcb_point_t * c = &jbxvt.scr.current->cursor;
	/* Sanitize non-relative arguments--must be positive.  */
	c->x = relative & COL_RELATIVE ? c->x + x : MAX(x, 0);
	c->y = relative & ROW_RELATIVE ? c->y + y : MAX(y, 0);
	reset_row_col();
	jbxvt.scr.current->wrap_next = 0;
	check_selection(c->y, c->y);
	draw_cursor(); // draw
}

