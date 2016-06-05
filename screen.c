/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.  */

#include "screen.h"

#include "change_offset.h"
#include "command.h"
#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "repaint.h"
#include "repair_damage.h"
#include "sbar.h"
#include "scroll.h"
#include "selection.h"
#include "selst.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "screenst.h"
#include "slinest.h"
#include "ttyinit.h"
#include "xsetup.h"
#include "xvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>


/*  Perform any initialisation on the screen data structures.
    Called just once at startup. */
void scr_init(void)
{
	// Initialise the array of lines that have scrolled off the top.
	jbxvt.scr.sline.max = DEF_SAVED_LINES;
	if (jbxvt.scr.sline.max < MAX_SCROLL)
		jbxvt.scr.sline.max = MAX_SCROLL;
	jbxvt.scr.sline.data = calloc(jbxvt.scr.sline.max,
		sizeof(void*));
	jbxvt.scr.s1.wrap = jbxvt.scr.s2.wrap=1;
	jbxvt.scr.current = &jbxvt.scr.s1;
	scr_reset();
}

//  Change between the alternate and the main screens
void scr_change_screen(const bool mode_high)
{
	home_screen();
	jbxvt.scr.current = mode_high
		? &jbxvt.scr.s2 : &jbxvt.scr.s1;
	jbxvt.sel.end2.se_type = NOSEL;
	repaint((xcb_point_t){}, (xcb_point_t){.y = jbxvt.scr.chars.height - 1,
		.x = jbxvt.scr.chars.width - 1});
	cursor(CURSOR_DRAW);
}

//  Change the rendition style.
void scr_change_rendition(const uint32_t style)
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
	if (jbxvt.scr.current->cursor.y == (mod > 0
		? jbxvt.scr.current->margin.bottom
		: jbxvt.scr.current->margin.top))
		scroll(jbxvt.scr.current->margin.top,
			jbxvt.scr.current->margin.bottom, mod);
	else
		jbxvt.scr.current->cursor.y += mod;
	jbxvt.scr.current->wrap_next = 0;
	check_selection(jbxvt.scr.current->cursor.y,
		jbxvt.scr.current->cursor.y);
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
	if (count > jbxvt.scr.current->margin.bottom
		- jbxvt.scr.current->cursor.y + 1)
		return;
	hsc();
	scroll(jbxvt.scr.current->cursor.y,
		jbxvt.scr.current->margin.bottom,
		scroll_up_scr_bot(count, true));
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

static void scroll_lower_lines(const int8_t count)
{
	scroll(jbxvt.scr.current->cursor.y,
		jbxvt.scr.current->margin.bottom,
		scroll_up_scr_bot(count, false));
}

static inline int8_t get_insertion_count(const int8_t count)
{
	return constrain(count, jbxvt.scr.current->margin.bottom
		- jbxvt.scr.current->cursor.y + 1);
}

/*  Insert count blank lines at the current position
    and scroll the lower lines down.  */
void scr_insert_lines(const int8_t count)
{
	if (jbxvt.scr.current->cursor.y
		> jbxvt.scr.current->margin.bottom)
		return;
	hsc();
	scroll_lower_lines(get_insertion_count(count));
	jbxvt.scr.current->wrap_next = 0;
	cursor(CURSOR_DRAW);
}

//  Attempt to set the top and bottom scroll margins.
void scr_set_margins(const uint16_t top, const uint16_t bottom)
{
	const uint16_t b = constrain(bottom, jbxvt.scr.chars.height);

	if (top > b) return;

	jbxvt.scr.current->margin.top = top;
	jbxvt.scr.current->margin.bottom = b;
	scr_move(0,0,0);
}

//  Move the display by a distance represented by the value.
void scr_move_by(const int16_t y)
{
	change_offset(jbxvt.scr.offset - y/jbxvt.X.font_height);
}

//  Reposition the scrolled text so that the scrollbar is at the bottom.
void home_screen(void)
{
	if (jbxvt.scr.offset) {
		jbxvt.scr.offset = 0;
		xcb_point_t rc = { .y = jbxvt.scr.chars.height - 1,
			.x = jbxvt.scr.chars.width - 1
		};
		repaint((xcb_point_t){}, rc);
		cursor(CURSOR_DRAW);
		sbar_show(rc.y + jbxvt.scr.sline.top, 0, rc.y);
	}
}

