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

static struct screenst save_screen;

/*  Perform any initialisation on the screen data structures.  Called just once
 *  at startup. */
void scr_init(void)
{
	// Initialise the array of lines that have scrolled off the top.
	jbxvt.scr.sline.max = DEF_SAVED_LINES;
	if (jbxvt.scr.sline.max < MAX_SCROLL)
		jbxvt.scr.sline.max = MAX_SCROLL;
	jbxvt.scr.sline.data = calloc(jbxvt.scr.sline.max,
		sizeof(void*));
	jbxvt.scr.s1.wrap=jbxvt.scr.s2.wrap=1;
	jbxvt.scr.current = &jbxvt.scr.s1;
	jbxvt.X.font_width = XTextWidth(jbxvt.X.font,"M",1);
	jbxvt.X.font_height = jbxvt.X.font->ascent
		+ jbxvt.X.font->descent;
	scr_reset();
}

//  Change between the alternate and the main screens
void scr_change_screen(const uint8_t direction)
{
	home_screen();
	jbxvt.scr.current = (direction == JBXVT_MODE_HIGH)
		? &jbxvt.scr.s2 : &jbxvt.scr.s1;
	jbxvt.sel.end2.se_type = NOSEL;
	repaint(0,jbxvt.scr.chars.height - 1,0,jbxvt.scr.chars.width - 1);
	cursor();
}

//  Change the rendition style.
void scr_change_rendition(const uint32_t style)
{
	// This allows combining styles, 0 resets
	jbxvt.scr.rstyle = style ? jbxvt.scr.rstyle | style : 0;
}

//  Return the width and height of the screen.
void scr_get_size(uint16_t * restrict width_p, uint16_t * restrict height_p)
{
	*width_p = jbxvt.scr.chars.width;
	*height_p = jbxvt.scr.chars.height;
}


//  Return true if the character is one that can be handled by scr_string()
int16_t is_string_char(int16_t c)
{
	c &= 0177;
	return(c >= ' ' || c == '\n' || c == '\r' || c == '\t');
}

static void hsc(void)
{
	home_screen();
	cursor();
}

/* Move the cursor up if mod is positive or down if mod is negative,
   by mod number of lines and scroll if necessary.  */
void scr_index_by(const int8_t mod)
{
	hsc();
	if (jbxvt.scr.current->row == (mod > 0 ? jbxvt.scr.current->bmargin
		: jbxvt.scr.current->tmargin))
		scroll(jbxvt.scr.current->tmargin,
			jbxvt.scr.current->bmargin, mod);
	else
		jbxvt.scr.current->row += mod;
	jbxvt.scr.current->wrap_next = 0;
	check_selection(jbxvt.scr.current->row,jbxvt.scr.current->row);
	cursor();

}

//  Save the cursor position and rendition style.
void scr_save_cursor(void)
{
	save_screen.row = jbxvt.scr.current->row;
	save_screen.col = jbxvt.scr.current->col;
	jbxvt.scr.saved_rstyle = jbxvt.scr.rstyle;
}

static void adj_cursor_wh(int16_t * restrict grc,
	int16_t src, int16_t chw)
{
	*grc = src;
	if (*grc >= chw)
		  *grc = chw - 1;
}

//  Restore the cursor position and rendition style.
void scr_restore_cursor(void)
{
	cursor();
	adj_cursor_wh(&jbxvt.scr.current->row, save_screen.row,
		jbxvt.scr.chars.height);
	adj_cursor_wh(&jbxvt.scr.current->col, save_screen.col,
		jbxvt.scr.chars.width);
	scr_change_rendition(jbxvt.scr.saved_rstyle);
	cursor();
}

//  Delete count lines and scroll up the bottom of the screen to fill the gap
void scr_delete_lines(uint8_t count)
{
	if (count > jbxvt.scr.current->bmargin
		- jbxvt.scr.current->row + 1)
		return;

	hsc();
	while (count > MAX_SCROLL) {
		scroll(jbxvt.scr.current->row,
			jbxvt.scr.current->bmargin,
			MAX_SCROLL);
		count -= MAX_SCROLL;
	}
	scroll(jbxvt.scr.current->row,jbxvt.scr.current->bmargin,count);
	jbxvt.scr.current->wrap_next = 0;
	cursor();
}

static void scroll_lower_lines(int8_t count)
{
	while (count > MAX_SCROLL) {
		scroll(jbxvt.scr.current->row,
			jbxvt.scr.current->bmargin,
			-MAX_SCROLL);
		count -= MAX_SCROLL;
	}
	scroll(jbxvt.scr.current->row,
		jbxvt.scr.current->bmargin,-count);
}

static int8_t get_insertion_count(const int8_t count)
{
	if (count > jbxvt.scr.current->bmargin
		- jbxvt.scr.current->row + 1)
		return jbxvt.scr.current->bmargin
			- jbxvt.scr.current->row + 1;
	return count;
}

/*  Insert count blank lines at the current position and scroll the lower lines
 *  down.  */
void scr_insert_lines(int8_t count)
{
	if (jbxvt.scr.current->row > jbxvt.scr.current->bmargin)
		return;
	count = get_insertion_count(count);
	hsc();
	scroll_lower_lines(count);
	jbxvt.scr.current->wrap_next = 0;
	cursor();
}

//  Attempt to set the top ans bottom scroll margins.
void scr_set_margins(const uint16_t top, const uint16_t bottom)
{
	const uint16_t b = bottom >= jbxvt.scr.chars.height
		? jbxvt.scr.chars.height - 1 : bottom;
	if (top > b) return;

	jbxvt.scr.current->tmargin = top;
	jbxvt.scr.current->bmargin = b;
	scr_move(0,0,0);
}

/*  Move the display so that line represented by scrollbar value y is at the top
 *  of the screen.  */
void scr_move_to(int16_t y)
{
	y = jbxvt.scr.pixels.height - 1 - y;
	const int16_t lnum = y * (jbxvt.scr.chars.height
		+ jbxvt.scr.sline.top - 1)
		/ (jbxvt.scr.pixels.height - 1);
	change_offset(lnum - jbxvt.scr.chars.height + 1);
}

//  Move the display by a distance represented by the value.
void scr_move_by(const int16_t y)
{
	change_offset(y >= 0
		? jbxvt.scr.offset - (y - MARGIN) / jbxvt.X.font_height
		: jbxvt.scr.offset + (-y - MARGIN) / jbxvt.X.font_height);
}

//  Send the name of the current display to the command.
void scr_report_display(void)
{
	char * restrict dname = DisplayString(jbxvt.X.dpy);
	struct utsname ut;
#ifdef SYS_uname
	syscall(SYS_uname, &ut);
#else//!SYS_uname
	uname(&ut);
#endif//SYS_uname

	if (!strncmp(dname, "unix:", 5))
		cprintf("%s%s\r",ut.nodename,dname + 4);
	else if (dname[0] == ':')
		cprintf("%s%s\r",ut.nodename,dname);
	else
		cprintf("%s\r",dname);
}

//  Report the current cursor position.
void scr_report_position(void)
{
	cprintf("\033[%d;%dR",jbxvt.scr.current->row + 1,
		jbxvt.scr.current->col + 1);
}

//  Reposition the scrolled text so that the scrollbar is at the bottom.
void home_screen(void)
{
	if (jbxvt.scr.offset > 0) {
		jbxvt.scr.offset = 0;
		repaint(0,jbxvt.scr.chars.height - 1,0,
			jbxvt.scr.chars.width - 1);
		cursor();
		sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1,
			0, jbxvt.scr.chars.height - 1);
	}
}

