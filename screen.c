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
	jbxvt.scr.sline.max = DEF_SAVED_LINES;
	if (jbxvt.scr.sline.max < MAX_SCROLL)
		jbxvt.scr.sline.max = MAX_SCROLL;
	jbxvt.scr.sline.data = GC_MALLOC(jbxvt.scr.sline.max * sizeof(void*));
	jbxvt.scr.s1.decawm = jbxvt.scr.s2.decawm = true;
	jbxvt.scr.s1.dectcem = jbxvt.scr.s2.dectcem = true;
	jbxvt.scr.current = &jbxvt.scr.s1;
	jbxvt.scr.s1.charset[0] = jbxvt.scr.s2.charset[0]
		= CHARSET_ASCII;
	jbxvt.scr.s1.charset[1] = jbxvt.scr.s2.charset[1]
		= CHARSET_SG0;
	scr_reset();
}

//  Change between the alternate and the main screens
void scr_change_screen(const bool mode_high)
{
	home_screen();
	jbxvt.scr.current = mode_high
		? &jbxvt.scr.s2 : &jbxvt.scr.s1;
	jbxvt.sel.end2.se_type = NOSEL;
	jbxvt.scr.sline.top = 0;
	repaint((xcb_point_t){}, (xcb_point_t){
		.y = jbxvt.scr.chars.height - 1,
		.x = jbxvt.scr.chars.width - 1});
	cursor(CURSOR_DRAW);
	scr_erase_screen(2); // ENTIRE
}

//  Change the rendition style.
void scr_style(const uint32_t style)
{
	// This allows combining styles, 0 resets
	jbxvt.scr.rstyle = style ? jbxvt.scr.rstyle | style : 0;
}

static void hsc(void)
{
	home_screen();
	cursor(CURSOR_DRAW);
}

/* Move the cursor up if mod is positive or down if mod is negative,
   by mod number of lines and scroll if necessary.  */
void scr_index_by(const int8_t mod)
{
	hsc();
	const Size m = jbxvt.scr.current->margin;
	scroll(m.top, m.bottom, mod);
	cursor(CURSOR_DRAW);
}

static int8_t scroll_up_scr_bot(uint8_t count, const bool up)
{
	while (count > MAX_SCROLL) {
		scroll(jbxvt.scr.current->cursor.y,
			jbxvt.scr.current->margin.bottom,
			up?MAX_SCROLL:-MAX_SCROLL);
		count -= MAX_SCROLL;
	}
	return up?count:-count;
}

//  Delete count lines and scroll up the bottom of the screen to fill the gap
void scr_delete_lines(const uint8_t count)
{
	VTScreen * s = jbxvt.scr.current;
	const uint8_t mb = s->margin.bottom;
	const int16_t cy = s->cursor.y;
	if (count > mb - cy + 1)
		return;
	hsc();
	scroll(cy, mb, scroll_up_scr_bot(count, true));
	s->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

static void scroll_lower_lines(const int8_t count)
{
	scroll(jbxvt.scr.current->cursor.y,
		jbxvt.scr.current->margin.bottom,
		scroll_up_scr_bot(count, false));
}

static inline uint8_t get_insertion_count(const int8_t count)
{
	const uint8_t lim = jbxvt.scr.current->margin.bottom
		- jbxvt.scr.current->cursor.y;
	return unlikely(count < 0) ? 0
		: unlikely(count > lim) ? lim : count;
}

/*  Insert count blank lines at the current position
    and scroll the lower lines down.  */
void scr_insert_lines(const int8_t count)
{
	hsc();
	scroll_lower_lines(get_insertion_count(count));
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

//  Reposition the scrolled text so that the scrollbar is at the bottom.
void home_screen(void)
{
	if (likely(!jbxvt.scr.offset))
		  return;
	jbxvt.scr.offset = 0;
	xcb_point_t rc = { .y = jbxvt.scr.chars.height - 1,
		.x = jbxvt.scr.chars.width - 1 };
	repaint((xcb_point_t){}, rc);
	cursor(CURSOR_DRAW);
	sbar_show(rc.y + jbxvt.scr.sline.top, 0, rc.y);
}

