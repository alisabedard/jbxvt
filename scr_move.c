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
	struct JBDim * c = &CUR;
	// Sanitize non-relative arguments--must be positive.
	set_dimension(&c->x, x, relative & COL_RELATIVE);
	set_dimension(&c->y, y, relative & ROW_RELATIVE);
	reset_row_col();
	const int16_t cy = c->y;
	SCR->wrap_next = 0;
	check_selection(cy, cy);
	draw_cursor(); // draw
}

