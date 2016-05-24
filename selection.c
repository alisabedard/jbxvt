/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selection.h"

#include "config.h"
#include "jbxvt.h"
#include "save_selection.h"
#include "selcmp.h"
#include "screen.h"
#include "slinest.h"
#include "show_selection.h"
#include "xsetup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>


// Static globals:
static enum selunit selection_unit;	/* current unit of selection */

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(const Time time)
{
	if (!save_selection())
		return;

	XSetSelectionOwner(jbxvt.X.dpy,XA_PRIMARY,jbxvt.X.win.vt,time);
	if (XGetSelectionOwner(jbxvt.X.dpy,XA_PRIMARY) != jbxvt.X.win.vt)
		fprintf(stderr, "Could not get primary selection");

	//  Place in CUT_BUFFER0 for backup.
	XChangeProperty(jbxvt.X.dpy,DefaultRootWindow(jbxvt.X.dpy),XA_CUT_BUFFER0,
		XA_STRING,8,PropModeReplace,jbxvt.sel.text,jbxvt.sel.length);
}

//  respond to a request for our current selection.
void scr_send_selection(const int time __attribute__((unused)),
	const int requestor, const int target, const int property)
{
	XEvent event = { .xselection.type = SelectionNotify,
		.xselection.selection = XA_PRIMARY, .xselection.target = XA_STRING,
		.xselection.requestor = requestor, .xselection.time = time };
	if (target == XA_STRING) {
		XChangeProperty(jbxvt.X.dpy,requestor,property,
			XA_STRING,8,PropModeReplace,
			jbxvt.sel.text,jbxvt.sel.length);
		event.xselection.property = property;
	} else
		event.xselection.property = None;
	XSendEvent(jbxvt.X.dpy,requestor,False,0,&event);
}

//  Clear the current selection.
void scr_clear_selection(void)
{
	if (jbxvt.sel.text != NULL) {
		free(jbxvt.sel.text);
		jbxvt.sel.text = NULL;
		jbxvt.sel.length = 0;
	}
	show_selection(0,jbxvt.scr.chars.height - 1,0,jbxvt.scr.chars.width - 1);
	jbxvt.sel.end1.se_type = jbxvt.sel.end2.se_type = NOSEL;
}

//  start a selection using the specified unit.
void scr_start_selection(int x, int y, enum selunit unit)
{
	show_selection(0,jbxvt.scr.chars.height - 1,0,jbxvt.scr.chars.width - 1);
	int16_t col = (x - MARGIN) / jbxvt.X.font_width;
	int16_t row = (y - MARGIN) / jbxvt.X.font_height;
	selection_unit = unit;
	fix_rc(&row,&col);
	rc_to_selend(row,col,&jbxvt.sel.anchor);
	jbxvt.sel.end2 = jbxvt.sel.end1 = jbxvt.sel.anchor;
	adjust_selection(&jbxvt.sel.end2);
	show_selection(0,jbxvt.scr.chars.height - 1,0,jbxvt.scr.chars.width - 1);
}


//  Convert a row and column coordinates into a selection endpoint.
void rc_to_selend(const int16_t row, const int16_t col, struct selst * se)
{
	int i = (row - jbxvt.scr.offset);
	if (i >= 0)
		se->se_type = SCREENSEL;
	else {
		se->se_type = SAVEDSEL;
		i = -1 - i;
	}
	se->se_index = i;
	se->se_col = col;
}

static int16_t constrain(const int16_t rc, const uint8_t lim)
{
	if (rc < 0)
		  return 0;
	else if (rc >= lim)
		  return lim - 1;
	return rc;
}


/*  Fix the coordinates so that they are within the screen and do not lie within
 *  empty space.
 */
void fix_rc(int16_t * restrict rowp, int16_t * restrict colp)
{
	int16_t col = constrain(*colp, jbxvt.scr.chars.width);
	int16_t row = constrain(*rowp, jbxvt.scr.chars.height);
	if (selection_unit == CHAR) {
		int i = (row - jbxvt.scr.offset);
		uint8_t * s;
		if (i >= 0) {
			s = jbxvt.scr.current->text[i];
			if (col > 0 && s[col - 1] < ' ')
				while (col < jbxvt.scr.chars.width && s[col] < ' ')
					col++;
		} else {
			i = -1 - i;
			const uint8_t len = jbxvt.scr.sline.data[i]->sl_length;
			s = jbxvt.scr.sline.data[i]->sl_text;
			if (col > 0 && s[col - 1] < ' ')
				while (col <= len && s[col] < ' ')
					col++;
			if (col > len)
				col = jbxvt.scr.chars.width;
		}
	}
	*colp = col;
	*rowp = row;
}

/*  Convert the selection into a row and column.
 */
void selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct selst * restrict se)
{
	if (se->se_type == NOSEL)
		return;

	*colp = se->se_col;
	*rowp = se->se_type == SCREENSEL ? se->se_index + jbxvt.scr.offset
		: jbxvt.scr.offset - se->se_index - 1;
}

/*  Convert a section of displayed text line into a text string suitable
    for pasting. *lenp is the length of the input string, i1 is index
    of the first character to convert and i2 is the last.  The length
    of the returned string is returned in *lenp; */
uint8_t * convert_line(uint8_t * restrict str,
	int * restrict lenp, uint8_t i1, uint8_t i2)
{
	// set this before i2 is modified
	const bool newline = (i2 + 1 == jbxvt.scr.chars.width)
		&& (str[*lenp] == 0);
	if (i2 >= *lenp)
		i2 = *lenp - 1;
	if (i2 - i1 >= MAX_WIDTH)
		i2 = i1 + MAX_WIDTH;
	while (i2 >= i1 && str[i2] == 0)
		i2--;
	static uint8_t buf[MAX_WIDTH + 3];
	uint8_t *s = buf;
	for (; i1 <= i2; i1++) {
		if (str[i1] >= ' ')
			*s++ = str[i1];
		else if (str[i1] == '\t') {
			*s++ = '\t';
			while (i1 < i2 && str[i1 + 1] == 0)
				i1++;
		} else
			*s++ = ' ';
	}
	if (newline)
		*s++ = '\n';
	*s = 0;
	*lenp = s - buf;
	return (buf);
}

static uint16_t sel_s(struct selst * restrict se2, uint8_t ** s)
{
	const bool ss = se2->se_type == SCREENSEL;
	*s = ss ? jbxvt.scr.current->text[se2->se_index]
		: jbxvt.scr.sline.data[se2->se_index]->sl_text;
	return ss ? jbxvt.scr.chars.width
		: jbxvt.scr.sline.data[se2->se_index]->sl_length;
}

static void adj_sel_to_word(struct selst * include,
	struct selst * se1, struct selst * se2)
{
	uint8_t * s = se1->se_type == SCREENSEL
		? jbxvt.scr.current->text[se1->se_index]
		: jbxvt.scr.sline.data[se1->se_index]->sl_text;
	int16_t i = se1->se_col;
	while (i && s[i] != ' ')
		  i--;
	se1->se_col = i?i+1:0;
	i = se2->se_col;
	if (se2 == include || !selcmp(se2,&jbxvt.sel.anchor))
		  i++;
	const uint16_t len = sel_s(se2, &s);
	while (i < len && s[i] && s[i] != ' ' && s[i] != '\n')
		  i++;
	se2->se_col = i;

}

/*  Adjust the selection to a word or line boundary. If the include endpoint is
 *  non NULL then the selection is forced to be large enough to include it.
 */
void adjust_selection(struct selst * restrict include)
{
	if (selection_unit == CHAR)
		return;
	struct selst *se1, *se2;
	if (selcmp(&jbxvt.sel.end1,&jbxvt.sel.end2) <= 0) {
		se1 = &jbxvt.sel.end1;
		se2 = &jbxvt.sel.end2;
	} else {
		se2 = &jbxvt.sel.end1;
		se1 = &jbxvt.sel.end2;
	}
	if (selection_unit == WORD)
		  adj_sel_to_word(include, se1, se2);
	else if (selection_unit == LINE) {
		se1->se_col = 0;
		se2->se_col = jbxvt.scr.chars.width;
	}
}

/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.
 */
void check_selection(const int16_t row1, const int16_t row2)
{

	if (jbxvt.sel.end1.se_type == NOSEL || jbxvt.sel.end2.se_type == NOSEL)
		return;
	int16_t r1 = jbxvt.sel.end1.se_type == SCREENSEL
		? jbxvt.sel.end1.se_index : -1;
	int16_t r2 = jbxvt.sel.end2.se_type == SCREENSEL
		? jbxvt.sel.end2.se_index : -1;
	if (r1 > r2) {
		int16_t x = r1;
		r1 = r2;
		r2 = x;
	}
	if (row2 < r1 || row1 > r2)
		return;
	show_selection(0, jbxvt.scr.chars.height - 1,
		0, jbxvt.scr.chars.width - 1);
	jbxvt.sel.end2.se_type = NOSEL;
}

