/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cursor.h"

#include "config.h"
#include "jbxvt.h"
#include "screen.h"

static xcb_point_t get_p(void)
{
	xcb_point_t p = jbxvt.scr.current->cursor;
	p.x *= jbxvt.X.font_size.width;
	p.y *= jbxvt.X.font_size.height;
	p.x += MARGIN;
	p.y += MARGIN;
	return p;
}

//  Draw the cursor at the current position.
static void draw_cursor(const uint8_t cursor_focus)
{
	// Don't draw cursor when scrolled
	if (jbxvt.scr.offset > 0)
		  return;
	if (!jbxvt.scr.current)
		  return; // prevent segfault
	if (!jbxvt.scr.current->dectcem) // hide cursor
		  return;
	xcb_point_t p = get_p();
	xcb_poly_fill_rectangle(jbxvt.X.xcb, jbxvt.X.win.vt,
		jbxvt.X.gc.cu, cursor_focus?1:2, (xcb_rectangle_t[]){
		{p.x, p.y, jbxvt.X.font_size.width, jbxvt.X.font_size.height},
		{p.x + 1, p.y + 1, jbxvt.X.font_size.width - 2,
		jbxvt.X.font_size.height - 2}});
}

__attribute__((nonnull(1)))
static inline void adj_wh(int16_t * restrict grc,
	int16_t src, uint16_t chw)
{
	*grc = src >= chw ? chw - 1 : src;
}

//  Restore the cursor position and rendition style.
static void restore(struct screenst * restrict s, const uint32_t r)
{
	if (!jbxvt.scr.current)
		  return;
	cursor(CURSOR_DRAW);
	adj_wh(&jbxvt.scr.current->cursor.y, s->cursor.y,
		jbxvt.scr.chars.height);
	adj_wh(&jbxvt.scr.current->cursor.x, s->cursor.x,
		jbxvt.scr.chars.width);
	scr_style(r);
	cursor(CURSOR_DRAW);
}

/*  Indicate a change of keyboard focus.  Type is 1 if focusing in,
    2 for entry events, and 4 for focus events.  */
static inline bool focus(const bool in, bool cursor_focus)
{
	draw_cursor(cursor_focus); // clear via invert gc
	draw_cursor(in);
	return in;
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
	case CURSOR_FOCUS_IN:
	case CURSOR_ENTRY_IN:
		cursor_focus = focus(true, cursor_focus);
		break;
	case CURSOR_FOCUS_OUT:
	case CURSOR_ENTRY_OUT:
		cursor_focus = focus(false, cursor_focus);
		break;
	case CURSOR_RESTORE:
		restore(&saved_screen, saved_rstyle);
		break;
	case CURSOR_SAVE: // Save the cursor position and rendition style.
		saved_screen.cursor.y = jbxvt.scr.current->cursor.y;
		saved_screen.cursor.x = jbxvt.scr.current->cursor.x;
		saved_rstyle = jbxvt.scr.rstyle;
		break;
	case CURSOR_REPORT: // Report the current cursor position.
		cprintf("\033[%d;%dR",jbxvt.scr.current->cursor.y + 1,
			jbxvt.scr.current->cursor.x + 1);
		break;
	}
}

