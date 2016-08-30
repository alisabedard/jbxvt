#include "scr_move.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "scroll.h"
#include "scr_reset.h"
#include "selection.h"

#include <stdlib.h>

#define CSZ jbxvt.scr.chars
#define SCR jbxvt.scr.current
#define CUR SCR->cursor
#define MGN SCR->margin

// Sanitize cursor position, implement DECOM
void reset_row_col(void)
{
	struct JBDim * c = &CUR;
	struct JBDim sz = CSZ;
	--sz.w; --sz.h;
	JB_LIMIT(c->x, sz.w, 0);
	JB_LIMIT(c->y, sz.h, 0);
	// Implement DECOM, DEC Origin Mode, limits
	if (jbxvt.mode.decom) {
		const struct JBDim m = MGN;
		JB_LIMIT(c->y, m.t, m.b);
	}
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
	struct JBDim * c = &CUR;
	// Sanitize non-relative arguments--must be positive.
	c->x = relative & COL_RELATIVE ? c->x + x : MAX(x, 0);
	const int16_t y = c->y = relative & ROW_RELATIVE
		? c->y + y : MAX(y, 0);
	reset_row_col();
	SCR->wrap_next = 0;
	check_selection(y, y);
	draw_cursor(); // draw
}

