#include "scr_move.h"

#include "cursor.h"
#include "global.h"
#include "screen.h"
#include "selection.h"

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.
 */
void scr_move(int x, int y, int relative)
{
	home_screen();
	cursor();
	screen->col = (relative & COL_RELATIVE) ? screen->col + x : x;
	if (screen->col < 0)
		screen->col = 0;
	if (screen->col >= cwidth)
		screen->col = cwidth - 1;

	if (relative & ROW_RELATIVE) {
		if (y > 0) {
			if (screen->row <= screen->bmargin
				&& screen->row + y > screen->bmargin)
				screen->row = screen->bmargin;
			else
				screen->row += y;
		} else if (y < 0) {
			if (screen->row >= screen->tmargin
				&& screen->row + y < screen->tmargin)
				screen->row = screen->tmargin;
			else
				screen->row += y;
		}
	} else {
		if (screen->decom) {
			screen->row = y + screen->tmargin;
			if (screen->row > screen->bmargin)
				screen->row = screen->bmargin;
		} else
			screen->row = y;
	}
	if (screen->row < 0)
		screen->row = 0;
	if (screen->row >= cheight)
		screen->row = cheight - 1;

	screen->wrap_next = 0;
	check_selection(screen->row,screen->row);
	cursor();
}

