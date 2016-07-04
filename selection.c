/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selection.h"

#include "config.h"
#include "jbxvt.h"
#include "save_selection.h"
#include "selcmp.h"
#include "screen.h"
#include "show_selection.h"
#include "xsetup.h"

#include <gc.h>
#include <stdlib.h>

// Static globals:
static enum selunit selection_unit;	/* current unit of selection */

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(const xcb_time_t time)
{
	save_selection();
	xcb_set_selection_owner(jbxvt.X.xcb, jbxvt.X.win.vt,
		XCB_ATOM_PRIMARY, time);
	// root cut buffer
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		jbxvt.X.screen->root, XCB_ATOM_CUT_BUFFER0, XCB_ATOM_STRING,
		8, jbxvt.sel.length, jbxvt.sel.text);
}

//  respond to a request for our current selection.
void scr_send_selection(const xcb_time_t time,
	const uint32_t requestor, const uint32_t target, const uint32_t property)
{
	// x events must be 32 bytes long:
	xcb_selection_notify_event_t e = {
		.response_type = XCB_SELECTION_NOTIFY,
		.selection = XCB_ATOM_PRIMARY, .target = target,
		.requestor = requestor, .time = time, .property = property};
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE, requestor,
		property, XCB_ATOM_STRING, 8, jbxvt.sel.length,
		jbxvt.sel.text);
	xcb_send_event(jbxvt.X.xcb, false, requestor,
		XCB_SELECTION_NOTIFY, (char *)&e);
	// send it before e looses scope
	xcb_flush(jbxvt.X.xcb);
}

//  Clear the current selection.
void scr_clear_selection(void)
{
	if (jbxvt.sel.text)
		jbxvt.sel.length = 0;
	const Size c = jbxvt.scr.chars;
	show_selection(0, c.h - 1, 0, c.w - 1);
	jbxvt.sel.end1.type = jbxvt.sel.end2.type = NOSEL;
}

//  start a selection using the specified unit.
void scr_start_selection(xcb_point_t p, enum selunit unit)
{
	const Size c = jbxvt.scr.chars;
	const Size f = jbxvt.X.font_size;
	show_selection(0, c.h - 1, 0, c.w - 1);
	xcb_point_t rc = { .x = (p.x - MARGIN) / f.w,
		.y = (p.y - MARGIN) / f.h};
	selection_unit = unit;
	fix_rc(&rc);
	rc_to_selend(rc.y, rc.x, &jbxvt.sel.anchor);
	jbxvt.sel.end2 = jbxvt.sel.end1 = jbxvt.sel.anchor;
	adjust_selection(&jbxvt.sel.end2);
	show_selection(0, c.h - 1, 0, c.w - 1);
}


//  Convert a row and column coordinates into a selection endpoint.
void rc_to_selend(const int16_t row, const int16_t col, SelEnd * se)
{
	int16_t i = (row - jbxvt.scr.offset);
	if (i >= 0)
		se->type = SCREENSEL;
	else {
		se->type = SAVEDSEL;
		i = -1 - i;
	}
	se->index = i;
	se->col = col;
}

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
	if (selection_unit == SEL_CHAR) {
		if (i > -1) {
			c = advance_c(c, jbxvt.scr.chars.width,
				jbxvt.scr.current->text[i]);
		} else {
			i = - 1 - i;
			c = advance_c(c, jbxvt.scr.sline.data[i]->sl_length,
				jbxvt.scr.sline.data[i]->sl_text);
		}
	}
	return c;
}

/*  Fix the coordinates so that they are within the screen and do not lie within
 *  empty space.  */
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

//  Convert the selection into a row and column.
void selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	SelEnd * restrict se)
{
	if (se->type == NOSEL)
		return;

	*colp = se->col;
	*rowp = se->type == SCREENSEL ? se->index + jbxvt.scr.offset
		: jbxvt.scr.offset - se->index - 1;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((pure,regparm(3)))
#else
	__attribute__((pure))
#endif
static inline uint8_t compute_i2(const uint16_t len, const uint8_t i1,
	uint8_t i2, uint8_t * restrict str)
{
	if (i2 >= len)
		i2 = len - 1;
	if (i2 - i1 >= MAX_WIDTH)
		i2 = i1 + MAX_WIDTH;
	while (i2 >= i1 && str[i2] == 0)
		--i2;
	return i2;
}

/*  Convert a section of displayed text line into a text string suitable
    for pasting. *lenp is the length of the input string, i1 is index
    of the first character to convert and i2 is the last.  The length
    of the returned string is returned in *lenp; */
uint8_t * convert_line(uint8_t * restrict str,
	uint16_t * restrict lenp, uint8_t i1, uint8_t i2)
{
	// set this before i2 is modified
	const bool newline = (i2 + 1 == jbxvt.scr.chars.width)
		&& (str[*lenp] == 0);
	i2 = compute_i2(*lenp, i1, i2, str);
	static uint8_t buf[MAX_WIDTH + 3];
	uint8_t *s = buf;
	for (; i1 <= i2; ++i1) {
		if (str[i1] >= ' ')
			*s++ = str[i1];
		else if (str[i1] == '\t') {
			*s++ = '\t';
			while (i1 < i2 && str[i1 + 1] == 0)
				++i1;
		} else
			*s++ = ' ';
	}
	if (newline)
		*s++ = '\n';
	*s = 0;
	*lenp = s - buf;
	return (buf);
}

static uint16_t sel_s(SelEnd * restrict se2, uint8_t ** s)
{
	const bool ss = se2->type == SCREENSEL;
	*s = ss ? jbxvt.scr.current->text[se2->index]
		: jbxvt.scr.sline.data[se2->index]->sl_text;
	return ss ? jbxvt.scr.chars.width
		: jbxvt.scr.sline.data[se2->index]->sl_length;
}

static void adj_sel_to_word(SelEnd * include,
	SelEnd * se1, SelEnd * se2)
{
	uint8_t * s = se1->type == SCREENSEL
		? jbxvt.scr.current->text[se1->index]
		: jbxvt.scr.sline.data[se1->index]->sl_text;
	int16_t i = se1->col;
	while (i && s[i] != ' ')
		  --i;
	se1->col = i?i+1:0;
	i = se2->col;
	if (se2 == include || !selcmp(se2,&jbxvt.sel.anchor))
		  ++i;
	const uint16_t len = sel_s(se2, &s);
	while (i < len && s[i] && s[i] != ' ' && s[i] != '\n')
		  ++i;
	se2->col = i;

}

/*  Adjust the selection to a word or line boundary. If the include endpoint is
 *  non NULL then the selection is forced to be large enough to include it.
 */
void adjust_selection(SelEnd * restrict include)
{
	if (selection_unit == SEL_CHAR)
		return;
	SelEnd *se1, *se2;
	const bool oneless = selcmp(&jbxvt.sel.end1,&jbxvt.sel.end2) <= 0;
	se1 = oneless ? &jbxvt.sel.end1 : &jbxvt.sel.end2;
	se2 = oneless ? &jbxvt.sel.end2 : &jbxvt.sel.end1;
	if (selection_unit == SEL_WORD)
		  adj_sel_to_word(include, se1, se2);
	else if (selection_unit == SEL_LINE) {
		se1->col = 0;
		se2->col = jbxvt.scr.chars.width;
	}
}

/*  Determine if the current selection overlaps row1-row2 and if it does then
 *  remove it from the screen.  */
void check_selection(const int16_t row1, const int16_t row2)
{
	if (jbxvt.sel.end1.type == NOSEL || jbxvt.sel.end2.type == NOSEL)
		return;
	int16_t r1 = jbxvt.sel.end1.type == SCREENSEL
		? jbxvt.sel.end1.index : -1;
	int16_t r2 = jbxvt.sel.end2.type == SCREENSEL
		? jbxvt.sel.end2.index : -1;
	if (r1 > r2) {
		const int16_t x = r1;
		r1 = r2;
		r2 = x;
	}
	if (row2 < r1 || row1 > r2)
		return;
	show_selection(0, jbxvt.scr.chars.height - 1,
		0, jbxvt.scr.chars.width - 1);
	jbxvt.sel.end2.type = NOSEL;
}

