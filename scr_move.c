#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "screen.h"
#include "scr_reset.h"
#include "selection.h"

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.
 */

void scr_move(const int16_t x, const int16_t y, const uint8_t relative)
{
	home_screen();
	cursor(CURSOR_DRAW);
	xcb_point_t * restrict c = &jbxvt.scr.current->cursor;
	c->x = (relative & COL_RELATIVE) ? c->x + x : x;
	if (relative & ROW_RELATIVE) {
		if (y > 0) {
			if (c->y <= jbxvt.scr.current->margin.bottom
				&& c->y + y > jbxvt.scr.current->margin.bottom)
				c->y = jbxvt.scr.current->margin.bottom;
			else
				c->y += y;
		} else if (y < 0) {
			if (c->y >= jbxvt.scr.current->margin.top && c->y + y
				< jbxvt.scr.current->margin.top)
				c->y = jbxvt.scr.current->margin.top;
			else
				c->y += y;
		}
	} else {
		if (jbxvt.scr.current->decom) {
			c->y = y + jbxvt.scr.current->margin.top;
			if (c->y > jbxvt.scr.current->margin.bottom)
				c->y = jbxvt.scr.current->margin.bottom;
		} else
			c->y = y;
	}
	reset_row_col();
	jbxvt.scr.current->wrap_next = 0;
	check_selection(c->y, c->y);
	cursor(CURSOR_DRAW);
}

