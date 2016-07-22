/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.  */

#include "screen.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "scroll.h"

#include <gc.h>

#if defined(__i386__) || defined(__amd64__)
__attribute__((regparm(2)))
#endif//__i386__||__amd64__
static uint8_t advance_c(uint8_t c, const uint8_t len,
	uint8_t * restrict s)
{
	if (c && s[c - 1] < ' ')
		while (c < len && s[c] < ' ')
			c++;
	if (c > len)
		c = jbxvt.scr.chars.width;
	return c;
}

#if defined(__i386__) || defined(__amd64__)
__attribute__((regparm(2)))
#endif//__i386__||__amd64__
static uint8_t find_c(uint8_t c, int16_t i)
{
	return jbxvt.sel.unit == SEL_CHAR
		? ipos(&i)
		? advance_c(c, jbxvt.scr.chars.width,
			    jbxvt.scr.current->text[i])
		: advance_c(c, jbxvt.scr.sline.data[i]->sl_length,
			    jbxvt.scr.sline.data[i]->sl_text)
		: c;
}

/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void fix_rc(xcb_point_t * restrict rc)
{
	const Size c = jbxvt.scr.chars;
	if(!c.h || !c.w)
		  return; // prevent segfault on bad window size.
	rc->x = MAX(rc->x, 0);
	rc->x = MIN(rc->x, c.w - 1);
	rc->y = MAX(rc->y, 0);
	rc->y = MIN(rc->y, c.h - 1);
	rc->x = find_c(rc->x, rc->y - jbxvt.scr.offset);
}

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
	repaint();
}

/*  Perform any initialization on the screen data structures.
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
	scr_set_tab(-2, false);
}

//  Change between the alternate and the main screens
void scr_change_screen(const bool mode_high)
{
	change_offset(0);
	jbxvt.scr.current = mode_high
		? &jbxvt.scr.s2 : &jbxvt.scr.s1;
	jbxvt.sel.end2.type = NOSEL;
	jbxvt.scr.sline.top = 0;
	repaint();
	cursor(CURSOR_DRAW);
	scr_erase_screen(2); // ENTIRE
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
	cursor(CURSOR_DRAW);
	scroll(top, jbxvt.scr.current->margin.b, count);
	cursor(CURSOR_DRAW);
}

