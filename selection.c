/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selection.h"

#include "config.h"
#include "jbxvt.h"
#include "save_selection.h"
#include "selcmp.h"
#include "show_selection.h"

// Static globals:
static enum selunit selection_unit;	/* current unit of selection */

static void prop(const xcb_window_t win, const xcb_atom_t a)
{
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE,
		win, a, XCB_ATOM_STRING, 8, jbxvt.sel.length, jbxvt.sel.text);
}

//  Make the selection currently delimited by the selection end markers.
void scr_make_selection(void)
{
	save_selection();
	/* Set all properties which may possibly be requested.  */
	const xcb_window_t v = jbxvt.X.win.vt;
	prop(v, XCB_ATOM_PRIMARY);
	prop(v, XCB_ATOM_SECONDARY);
	prop(v, jbxvt.X.clipboard);
	prop(jbxvt.X.screen->root, XCB_ATOM_CUT_BUFFER0);
	xcb_set_selection_owner(jbxvt.X.xcb, v, XCB_ATOM_PRIMARY,
		XCB_CURRENT_TIME);
}

//  Respond to a request for our current selection.
void scr_send_selection(const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property)
{
	// x events must be 32 bytes long:
	xcb_selection_notify_event_t e = {
		.response_type = XCB_SELECTION_NOTIFY,
		.selection = XCB_ATOM_PRIMARY, .target = target,
		.requestor = requestor, .time = time, .property = property};
	// If property is None, use taget, per ICCCM
	xcb_change_property(jbxvt.X.xcb, XCB_PROP_MODE_REPLACE, requestor,
		property == XCB_NONE ? target : property,
		XCB_ATOM_STRING, 8, jbxvt.sel.length,
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

static bool ipos(int16_t * i)
{
	if (*i < 0) {
		*i = -1 - *i;
		return false;
	}
	return true;
}

//  Convert a row and column coordinates into a selection endpoint.
void rc_to_selend(const int16_t row, const int16_t col, SelEnd * se)
{
	int16_t i = (row - jbxvt.scr.offset);
	se->type = ipos(&i) ? SCREENSEL : SAVEDSEL;
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
	return selection_unit == SEL_CHAR
		? ipos(&i)
		? advance_c(c, jbxvt.scr.chars.width,
			    jbxvt.scr.current->text[i])
		: advance_c(c, jbxvt.scr.sline.data[i]->sl_length,
			    jbxvt.scr.sline.data[i]->sl_text)
		: c;
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
	const SelEnd *e1 = &jbxvt.sel.end1, *e2 = &jbxvt.sel.end2;
	if (e1->type == NOSEL || e2->type == NOSEL)
		return;
	int16_t r1 = e1->type == SCREENSEL ? e1->index : -1;
	int16_t r2 = e2->type == SCREENSEL ? e2->index : -1;
	if (r1 > r2)
		SWAP(int16_t, r1, r2);
	if (row2 < r1 || row1 > r2)
		return;
	const Size c = jbxvt.scr.chars;
	show_selection(0, c.h - 1, 0, c.w - 1);
	jbxvt.sel.end2.type = NOSEL;
}

