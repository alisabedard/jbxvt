/*  Copyright 1992, 1994 John Bovey, University of Kent at Canterbury.
 *
 *  Redistribution and use in source code and/or executable forms, with
 *  or without modification, are permitted provided that the following
 *  condition is met:
 *
 *  Any redistribution must retain the above copyright notice, this
 *  condition and the following disclaimer, either as part of the
 *  program source code included in the redistribution or in human-
 *  readable materials provided with the redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS".  Any express or implied
 *  warranties concerning this software are disclaimed by the copyright
 *  holder to the fullest extent permitted by applicable law.  In no
 *  event shall the copyright-holder be liable for any damages of any
 *  kind, however caused and on any theory of liability, arising in any
 *  way out of the use of, or inability to use, this software.
 *
 *  -------------------------------------------------------------------
 *
 *  In other words, do not misrepresent my work as your own work, and
 *  do not sue me if it causes problems.  Feel free to do anything else
 *  you wish with it.
 */

#include "screen.h"

#include "change_offset.h"
#include "command.h"
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

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

static struct screenst save_screen;

/*  Perform any initialisation on the screen data structures.  Called just once
 *  at startup.  saved_lines is the number of saved lines.
 */
void scr_init(const unsigned int saved_lines)
{
	//  Initialise the array of lines that have scrolled of the top.
	jbxvt.scr.sline.max = saved_lines;
	if (jbxvt.scr.sline.max < MAX_SCROLL)
		jbxvt.scr.sline.max = MAX_SCROLL;
	jbxvt.scr.sline.data = (struct slinest **)
		malloc( jbxvt.scr.sline.max
		* sizeof(struct slinest *));
	for (uint16_t i = 0; i < jbxvt.scr.sline.max; i++)
		jbxvt.scr.sline.data[i] = NULL;
	jbxvt.scr.s1.wrap=jbxvt.scr.s2.wrap=1;
	jbxvt.scr.current = &jbxvt.scr.s1;
	jbxvt.X.font_width = XTextWidth(jbxvt.X.font,"M",1);
	jbxvt.X.font_height = jbxvt.X.font->ascent + jbxvt.X.font->descent;
	scr_reset();
}

//  Handle a backspace
void scr_backspace()
{
	scr_move(-1,0,COL_RELATIVE|ROW_RELATIVE);
}

/*  Ring the bell
 */
void scr_bell(void)
{
	XBell(jbxvt.X.dpy,0);
}

/*  Change between the alternate and the main screens
 */
void scr_change_screen(const uint8_t direction)
{
	home_screen();
	jbxvt.scr.current = (direction == HIGH)
		? &jbxvt.scr.s2 : &jbxvt.scr.s1;
	jbxvt.sel.end2.se_type = NOSEL;
	repaint(0,jbxvt.scr.chars.height - 1,0,jbxvt.scr.chars.width - 1);
	cursor();
}

/*  Change the rendition style.
 */
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

//  Move the cursor down one line and scroll if necessary.
void scr_index(void)
{
	home_screen();
	cursor();
	if (jbxvt.scr.current->row == jbxvt.scr.current->bmargin)
		scroll(jbxvt.scr.current->tmargin,jbxvt.scr.current->bmargin,1);
	else
		jbxvt.scr.current->row++;
	jbxvt.scr.current->wrap_next = 0;
	check_selection(jbxvt.scr.current->row,jbxvt.scr.current->row);
	cursor();
}

//  Move the cursor up one line and scroll if necessary.
void scr_rindex(void)
{
	home_screen();
	cursor();
	if (jbxvt.scr.current->row == jbxvt.scr.current->tmargin)
		scroll(jbxvt.scr.current->tmargin,jbxvt.scr.current->bmargin,-1);
	else
		jbxvt.scr.current->row--;
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

//  Restore the cursor position and rendition style.
void scr_restore_cursor(void)
{
	cursor();
	jbxvt.scr.current->row = save_screen.row;
	if (jbxvt.scr.current->row >= jbxvt.scr.chars.height)
		jbxvt.scr.current->row = jbxvt.scr.chars.height - 1;
	jbxvt.scr.current->col = save_screen.col;
	if (jbxvt.scr.current->col >= jbxvt.scr.chars.width)
		jbxvt.scr.current->col = jbxvt.scr.chars.width - 1;
	scr_change_rendition(jbxvt.scr.saved_rstyle);
	cursor();
}

//  Delete count lines and scroll up the bottom of the screen to fill the gap
void scr_delete_lines(int count)
{
	if (count > jbxvt.scr.current->bmargin
		- jbxvt.scr.current->row + 1)
		return;

	home_screen();
	cursor();
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

/*  Insert count blank lines at the current position and scroll the lower lines
 *  down.
 */
void scr_insert_lines(int count)
{
	if (jbxvt.scr.current->row > jbxvt.scr.current->bmargin)
		return;
	if (count > jbxvt.scr.current->bmargin
		- jbxvt.scr.current->row + 1)
		count = jbxvt.scr.current->bmargin
			- jbxvt.scr.current->row + 1;

	home_screen();
	cursor();
	while (count > MAX_SCROLL) {
		scroll(jbxvt.scr.current->row,
			jbxvt.scr.current->bmargin,
			-MAX_SCROLL);
		count -= MAX_SCROLL;
	}
	scroll(jbxvt.scr.current->row,jbxvt.scr.current->bmargin,-count);
	jbxvt.scr.current->wrap_next = 0;
	cursor();
}

//  Attempt to set the top ans bottom scroll margins.
void scr_set_margins(int16_t top, int16_t bottom)
{
	if (top < 0)
		top = 0;
	if (bottom >= jbxvt.scr.chars.height)
		bottom = jbxvt.scr.chars.height - 1;
	if (top > bottom)
		return;

	jbxvt.scr.current->tmargin = top;
	jbxvt.scr.current->bmargin = bottom;
	scr_move(0,0,0);
}

/*  Move the display so that line represented by scrollbar value y is at the top
 *  of the screen.
 */
void scr_move_to(int16_t y)
{
	int16_t n, lnum;

	y = jbxvt.scr.pixels.height - 1 - y;
	lnum = y * (jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1)
		/ (jbxvt.scr.pixels.height - 1);
	n = lnum - jbxvt.scr.chars.height + 1;
	change_offset(n);
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
	(void)uname(&ut);

	if (strncmp(dname, "unix:", 5) == 0)
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

