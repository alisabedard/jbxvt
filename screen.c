/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.  */

#include "screen.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "scroll.h"

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

// Renderless 'E' at position:
static inline void epos(const struct JBDim p)
{
	SCR->text[p.y][p.x] = 'E';
	SCR->rend[p.y][p.x] = 0;
}

// Set all chars to 'E'
void scr_efill(void)
{
	LOG("scr_efill");
	// Move to cursor home in order for all characters to appear.
	scr_move(0, 0, 0);
	struct JBDim p;
	const struct JBDim c = jbxvt.scr.chars;
	for (p.y = c.height - 1; p.y >= 0; --p.y)
		  for (p.x = c.width - 1; p.x >= 0; --p.x)
			    epos(p);
	repaint();
}

//  Change between the alternate and the main screens
void scr_change_screen(const bool mode_high)
{
	change_offset(0);
	jbxvt.scr.sline.top = 0;
	SCR = &jbxvt.scr.s[mode_high];
	jbxvt.sel.end[1].type = NOSEL;
	jbxvt.mode.charsel = 0; // reset on screen change
	draw_cursor();
	scr_erase_screen(2); // ENTIRE
	scr_reset();
}

//  Change the rendition style.
void scr_style(const enum RenderFlag style)
{
	// This allows combining styles, 0 resets
	jbxvt.scr.rstyle = style ? jbxvt.scr.rstyle | style : 0;
}

// Scroll from top to current bottom margin count lines, moving cursor
void scr_index_from(const int8_t count, const int16_t top)
{
	change_offset(0);
	draw_cursor();
	scroll(top, SCR->margin.b, count);
	draw_cursor();
}

