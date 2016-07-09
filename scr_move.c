#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"

#include <stdlib.h>

// Sanitize cursor position, implement DECOM
void reset_row_col(void)
{
	xcb_point_t * c = &jbxvt.scr.current->cursor;
	int16_t l = jbxvt.scr.chars.width - 1;
	c->x = MAX(MIN(c->x, l), 0);
	l = jbxvt.scr.chars.height - 1;
	c->y = MAX(MIN(c->y, l), 0);
	// Implement DECOM, DEC Origin Mode, limits
	if (jbxvt.scr.current->decom) {
		const Size m = jbxvt.scr.current->margin;
		c->y = MAX(MIN(c->y, m.top), m.bottom);
	}
}

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void scr_move(const int16_t x, const int16_t y, const uint8_t relative)
{
#ifdef MOVE_DEBUG
	LOG("scr_move(x:%d, y:%d, relative:%d)", x, y, relative);
#endif//MOVE_DEBUG
	home_screen();
	cursor(CURSOR_DRAW); // clear
	xcb_point_t * c = &jbxvt.scr.current->cursor;
	/* Sanitize non-relative arguments--must be positive.  */
	c->x = relative & COL_RELATIVE ? c->x + x : MAX(x, 0);
	c->y = relative & ROW_RELATIVE ? c->y + y : MAX(y, 0);
	reset_row_col();
	jbxvt.scr.current->wrap_next = 0;
	check_selection(c->y, c->y);
	cursor(CURSOR_DRAW); // draw
}

