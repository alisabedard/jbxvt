/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.  */

#include "screen.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "scroll.h"

#include <string.h>

#define GET_X(op) p.w op##= FSZ.w; p.y op##= FSZ.h; return p;

struct JBDim jbxvt_get_char_size(struct JBDim p)
{
	GET_X(/);
}

struct JBDim jbxvt_get_pixel_size(struct JBDim p)
{
	GET_X(*);
}

/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void fix_rc(struct JBDim * restrict rc)
{
	const struct JBDim c = jbxvt.scr.chars;
	if(!c.h || !c.w)
		  return; // prevent segfault on bad window size.
	JB_LIMIT(rc->x, c.w - 1, 0);
	JB_LIMIT(rc->y, c.h - 1, 0);
}

// Set all chars to 'E'
void jbxvt_efill(void)
{
	LOG("jbxvt_efill");
	// Move to cursor home in order for all characters to appear.
	jbxvt_move(0, 0, 0);
	for (uint8_t y = 0; y < CSZ.h; ++y) {
		memset(SCR->text[y], 'E', CSZ.w);
		memset(SCR->rend[y], 0, CSZ.w << 2);
	}
	repaint();
}

//  Change between the alternate and the main screens
void jbxvt_change_screen(const bool mode_high)
{
	change_offset(0);
	jbxvt.scr.sline.top = 0;
	SCR = &jbxvt.scr.s[mode_high];
	jbxvt_clear_selection();
	jbxvt.mode.charsel = 0; // reset on screen change
	draw_cursor();
	jbxvt_reset();
}

//  Change the rendition style.
void jbxvt_style(const enum RenderFlag style)
{
	// This allows combining styles, 0 resets
	jbxvt.scr.rstyle = style ? jbxvt.scr.rstyle | style : 0;
}

// Scroll from top to current bottom margin count lines, moving cursor
void jbxvt_index_from(const int8_t count, const int16_t top)
{
	change_offset(0);
	draw_cursor();
	scroll(top, SCR->margin.b, count);
	draw_cursor();
}

