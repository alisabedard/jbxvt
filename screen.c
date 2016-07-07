/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.  */

#include "screen.h"

#include "change_offset.h"
#include "config.h"
#include "cursor.h"
#include "log.h"
#include "repaint.h"
#include "sbar.h"
#include "scroll.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_refresh.h"
#include "scr_reset.h"
#include "selection.h"

#include <gc.h>
#include <stdlib.h>

// Renderless 'E' at position:
static void epos(const xcb_point_t p)
{
	VTScreen * restrict s = jbxvt.scr.current;
	s->text[p.y][p.x] = 'E';
	s->rend[p.y][p.x] = 0;
}

// Set all chars to 'E'
void scr_efill(void)
{
	// Move to cursor home in order for all characters to appear.
	scr_move(0, 0, 0);
	xcb_point_t p;
	const Size c = jbxvt.scr.chars;
	for (p.y = c.height - 1; p.y >= 0; --p.y)
		  for (p.x = c.width - 1; p.x >= 0; --p.x)
			    epos(p);
	const Size px = jbxvt.scr.pixels;
	scr_refresh((xcb_rectangle_t){.width = px.width,
		.height = px.height});
}

/*  Perform any initialisation on the screen data structures.
    Called just once at startup. */
void scr_init(void)
{
	// Initialise the array of lines that have scrolled off the top.
	jbxvt.scr.sline.max = MAX_SCROLL;
	jbxvt.scr.sline.data = GC_MALLOC(jbxvt.scr.sline.max * sizeof(void*));
#define SETBOTH(f, val) jbxvt.scr.s1.f = val; jbxvt.scr.s2.f = val;
	SETBOTH(decawm, true);
	SETBOTH(dectcem, true);
	SETBOTH(charset[0], CHARSET_ASCII);
	SETBOTH(charset[1], CHARSET_ASCII);
#undef SETBOTH
	jbxvt.scr.current = &jbxvt.scr.s1;
	scr_reset();
}

//  Change between the alternate and the main screens
void scr_change_screen(const bool mode_high)
{
	home_screen();
	jbxvt.scr.current = mode_high
		? &jbxvt.scr.s2 : &jbxvt.scr.s1;
	jbxvt.sel.end2.type = NOSEL;
	jbxvt.scr.sline.top = 0;
	const Size c = jbxvt.scr.chars;
	repaint((xcb_rectangle_t){.width = c.w - 1, .height = c.h - 1});
	cursor(CURSOR_DRAW);
	scr_erase_screen(2); // ENTIRE
}

//  Change the rendition style.
void scr_style(const uint32_t style)
{
	// This allows combining styles, 0 resets
	jbxvt.scr.rstyle = style ? jbxvt.scr.rstyle | style : 0;
}

// Scroll from top to current bottom margin count lines, moving cursor
void scr_index_from(const int8_t count, const int16_t top)
{
	home_screen();
	cursor(CURSOR_DRAW);
	scroll(top, jbxvt.scr.current->margin.b, count);
	cursor(CURSOR_DRAW);
}

//  Reposition the scrolled text so that the scrollbar is at the bottom.
void home_screen(void)
{
	if (likely(!jbxvt.scr.offset))
		  return;
	jbxvt.scr.offset = 0;
	xcb_rectangle_t r = { .height = jbxvt.scr.chars.height - 1,
		.width = jbxvt.scr.chars.width - 1 };
	repaint(r);
	cursor(CURSOR_DRAW);
	sbar_show(r.height + jbxvt.scr.sline.top, 0, r.height);
}

