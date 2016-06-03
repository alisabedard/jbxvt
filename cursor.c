/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cursor.h"

#include "config.h"
#include "jbxvt.h"
#include "screen.h"

//  Draw the cursor at the current position.
static void draw_cursor(const uint8_t cursor_focus)
{
	if (jbxvt.scr.offset > 0)
		return;

	if (!jbxvt.scr.current)
		  return; // prevent segfault
	Point p = jbxvt.scr.current->cursor;
	p.x *= jbxvt.X.font_width;
	p.y *= jbxvt.X.font_height;
	p.x += MARGIN;
	p.y += MARGIN;
	XFillRectangle(jbxvt.X.dpy, jbxvt.X.win.vt, jbxvt.X.gc.cu,
		p.x, p.y, jbxvt.X.font_width, jbxvt.X.font_height);
	if (!cursor_focus)
		XFillRectangle(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.cu,
			p.x + 1, p.y + 1, jbxvt.X.font_width - 2,
			jbxvt.X.font_height - 2);
}

static void adj_wh(int16_t * restrict grc,
	int16_t src, uint16_t chw)
{
	*grc = src >= chw ? chw - 1 : src;
}

//  Restore the cursor position and rendition style.
static void restore(struct screenst * restrict s, const uint32_t r)
{
	cursor(CURSOR_DRAW);
	adj_wh(&jbxvt.scr.current->cursor.row, s->cursor.row,
		jbxvt.scr.chars.height);
	adj_wh(&jbxvt.scr.current->cursor.col, s->cursor.col,
		jbxvt.scr.chars.width);
	scr_change_rendition(r);
	cursor(CURSOR_DRAW);
}

enum ScreenFocusFlags {
	SCR_FOCUS_IN = 1,
	SCR_FOCUS_ENTRY = 2,
	SCR_FOCUS_FOCUS = 4
};

/*  Indicate a change of keyboard focus.  Type is 1 if focusing in,
    2 for entry events, and 4 for focus events.  */
static void focus(const uint8_t flags, uint8_t * restrict cursor_focus)
{
	draw_cursor(*cursor_focus); // clear via invert gc
	if(flags & SCR_FOCUS_IN)
		  *cursor_focus |= flags>>1;
	else
		  *cursor_focus &= ~(flags>>1);
	draw_cursor(*cursor_focus); // draw
}

void cursor(const enum CursorOp op)
{
	static struct screenst saved_screen; // saved cursor position
	static uint32_t saved_rstyle; // saved render style
	static uint8_t cursor_focus; // window has focus if nonzero

	switch(op) {
	case CURSOR_DRAW:
		draw_cursor(cursor_focus);
		break;
	case CURSOR_ENTRY_IN:
		focus(SCR_FOCUS_IN|SCR_FOCUS_ENTRY, &cursor_focus);
		break;
	case CURSOR_ENTRY_OUT:
		focus(SCR_FOCUS_ENTRY, &cursor_focus);
		break;
	case CURSOR_FOCUS_IN:
		focus(SCR_FOCUS_IN|SCR_FOCUS_FOCUS, &cursor_focus);
		break;
	case CURSOR_FOCUS_OUT:
		focus(SCR_FOCUS_FOCUS, &cursor_focus);
		break;
	case CURSOR_RESTORE:
		restore(&saved_screen, saved_rstyle);
		break;
	case CURSOR_SAVE: // Save the cursor position and rendition style.
		saved_screen.cursor.row = jbxvt.scr.current->cursor.row;
		saved_screen.cursor.col = jbxvt.scr.current->cursor.col;
		saved_rstyle = jbxvt.scr.rstyle;
		break;
	case CURSOR_REPORT: // Report the current cursor position.
		cprintf("\033[%d;%dR",jbxvt.scr.current->cursor.row + 1,
			jbxvt.scr.current->cursor.col + 1);
		break;
	}
}

