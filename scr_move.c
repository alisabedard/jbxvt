#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "screen.h"
#include "selection.h"

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.
 */
void scr_move(int x, int y, int relative)
{
	home_screen();
	cursor();
	jbxvt.scr.current->col = (relative & COL_RELATIVE)
		? jbxvt.scr.current->col + x : x;
	if (jbxvt.scr.current->col < 0)
		jbxvt.scr.current->col = 0;
	if (jbxvt.scr.current->col >= jbxvt.scr.chars.width)
		jbxvt.scr.current->col = jbxvt.scr.chars.width - 1;

	if (relative & ROW_RELATIVE) {
		if (y > 0) {
			if (jbxvt.scr.current->row
				<= jbxvt.scr.current->bmargin
				&& jbxvt.scr.current->row + y
				> jbxvt.scr.current->bmargin)
				jbxvt.scr.current->row
					= jbxvt.scr.current->bmargin;
			else
				jbxvt.scr.current->row += y;
		} else if (y < 0) {
			if (jbxvt.scr.current->row
				>= jbxvt.scr.current->tmargin
				&& jbxvt.scr.current->row + y
				< jbxvt.scr.current->tmargin)
				jbxvt.scr.current->row
					= jbxvt.scr.current->tmargin;
			else
				jbxvt.scr.current->row += y;
		}
	} else {
		if (jbxvt.scr.current->decom) {
			jbxvt.scr.current->row = y
				+ jbxvt.scr.current->tmargin;
			if (jbxvt.scr.current->row
				> jbxvt.scr.current->bmargin)
				jbxvt.scr.current->row
					= jbxvt.scr.current->bmargin;
		} else
			jbxvt.scr.current->row = y;
	}
	if (jbxvt.scr.current->row < 0)
		jbxvt.scr.current->row = 0;
	if (jbxvt.scr.current->row >= jbxvt.scr.chars.height)
		jbxvt.scr.current->row = jbxvt.scr.chars.height - 1;

	jbxvt.scr.current->wrap_next = 0;
	check_selection(jbxvt.scr.current->row,
		jbxvt.scr.current->row);
	cursor();
}

