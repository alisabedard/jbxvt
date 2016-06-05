#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "screen.h"
#include "scr_reset.h"
#include "selection.h"

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.
 */
void scr_move(int x, int y, int relative)
{
	home_screen();
	cursor(CURSOR_DRAW);
	jbxvt.scr.current->cursor.x = (relative & COL_RELATIVE)
		? jbxvt.scr.current->cursor.x + x : x;
	reset_row_col();

	if (relative & ROW_RELATIVE) {
		if (y > 0) {
			if (jbxvt.scr.current->cursor.y
				<= jbxvt.scr.current->margin.bottom
				&& jbxvt.scr.current->cursor.y + y
				> jbxvt.scr.current->margin.bottom)
				jbxvt.scr.current->cursor.y
					= jbxvt.scr.current->margin.bottom;
			else
				jbxvt.scr.current->cursor.y += y;
		} else if (y < 0) {
			if (jbxvt.scr.current->cursor.y
				>= jbxvt.scr.current->margin.top
				&& jbxvt.scr.current->cursor.y + y
				< jbxvt.scr.current->margin.top)
				jbxvt.scr.current->cursor.y
					= jbxvt.scr.current->margin.top;
			else
				jbxvt.scr.current->cursor.y += y;
		}
	} else {
		if (jbxvt.scr.current->decom) {
			jbxvt.scr.current->cursor.y = y
				+ jbxvt.scr.current->margin.top;
			if (jbxvt.scr.current->cursor.y
				> jbxvt.scr.current->margin.bottom)
				jbxvt.scr.current->cursor.y
					= jbxvt.scr.current->margin.bottom;
		} else
			jbxvt.scr.current->cursor.y = y;
	}
	reset_row_col();

	jbxvt.scr.current->wrap_next = 0;
	check_selection(jbxvt.scr.current->cursor.y,
		jbxvt.scr.current->cursor.y);
	cursor(CURSOR_DRAW);
}

