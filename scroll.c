/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "scroll.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include <string.h>

//#define SCROLL_DEBUG
#ifdef SCROLL_DEBUG
#define SLOG(...) LOG(__VA_ARGS__)
#else
#define SLOG(...)
#endif

static bool move_selection(struct SelEnd * end, const uint16_t index,
	const uint16_t new_index)
{
	if (end->type == SCREENSEL && end->index == index) {
		end->type = SAVEDSEL;
		end->index = new_index;
		return true; // selection end point moved
	}
	return false; // no changes
}

static void move_line(const int16_t j,
	const int8_t count, struct JBXVTScreen * restrict s)
{
	const int16_t k = j + count;
	s->text[k] = s->text[j];
	s->rend[k] = s->rend[j];
	if (!move_selection(&jbxvt.sel.end[0], j, k))
		move_selection(&jbxvt.sel.end[1], j, k);
}

static void ck_sel_on_scr(const int16_t j)
{
	// clear selection if it scrolls off screen:
	if (jbxvt.sel.end[0].index == j
		|| jbxvt.sel.end[1].index == j)
		  scr_clear_selection();
}

static void clear(int8_t count, const uint8_t rc,
	uint8_t ** text, uint32_t ** rend, const bool up)
{
	if(--count < 0)
		  return;
	memset(text[count], 0, CSZ.w);
	memset(rend[count], 0, CSZ.w << 2);
	const uint8_t j = rc + (up ? - count - 1 : count);
	SCR->text[j] = text[count];
	SCR->rend[j] = rend[count];
	clear(count, rc, text, rend, up);
}

static void copy_saved_lines(const int_fast16_t n)
{
	for (int_fast16_t i = n - 1; i >= 0; --i) {
		uint8_t * t = SCR->text[i];
#define SLINE jbxvt.scr.sline
		const uint16_t new_index = n - i - 1;
		struct JBXVTSavedLine * sl = &SLINE.data[new_index];
		sl->wrap = SCR->wrap[i];
		SLINE.top += n;
		SLINE.top = MIN(SLINE.top, SLINE.max);
		if (!move_selection(&jbxvt.sel.end[0], i, new_index))
			move_selection(&jbxvt.sel.end[1], i, new_index);
		int_fast16_t len = 0;
		while(t[++len]); // strlen
		memcpy(sl->text, t, len);
		memcpy(sl->rend, SCR->rend[i], len << 2);
		sl->sl_length = len;
	}
}

static void get_y(int16_t * restrict y, const uint8_t row1,
	const int8_t count, const bool up)
{
	const uint8_t fh = jbxvt.X.f.size.h;
	const int16_t a = row1 * fh;
	const int16_t b = a + count * fh;
	*(up ? y + 1 : y) = a;
	*(up ? y : y + 1) = b;
}

static void copy_visible_area(const uint8_t row1, const uint8_t row2,
	const int8_t count, const bool up)
{
	int16_t y[2];
	get_y(y, row1, count, up);
	const uint16_t height = (row2 - row1 - count)
		* jbxvt.X.f.size.h;
	xcb_copy_area(jbxvt.X.xcb, jbxvt.X.win.vt,
		jbxvt.X.win.vt, jbxvt.X.gc.tx, 0, y[0],
		0, y[1], jbxvt.scr.pixels.width, height);
	// the above blocks the event queue, flush it
	xcb_flush(jbxvt.X.xcb);
}

// Handle lines that scroll off the top of the screen.
static void scroll_off(const int16_t count)
{
	// -1 to avoid going over array bounds
	memcpy(jbxvt.scr.sline.data + count,
		jbxvt.scr.sline.data, count - 1);
}

static void add_scroll_history(const int8_t count)
{
	scroll_off(count);
	int_fast16_t y = jbxvt.scr.sline.max - count - 1;
	struct JBXVTSavedLine * i = &jbxvt.scr.sline.data[y],
		* j = &jbxvt.scr.sline.data[y + count];
	for (; y >= 0; --y, --i, --j)
		memcpy(j, i, sizeof(struct JBXVTSavedLine));
	copy_saved_lines(count);
	sbar_draw(CSZ.h + jbxvt.scr.sline.top + count, jbxvt.scr.offset,
		CSZ.h);
}

static int8_t copy_screen_area(const int8_t i,
	const int8_t j, const int8_t mod, const int8_t count,
	uint8_t ** save, uint32_t ** rend)
{
	if(i >= count)
		  return j;
	save[i] = SCR->text[j];
	rend[i] = SCR->rend[j];
	ck_sel_on_scr(j);
	return copy_screen_area(i + 1, j + mod, mod, count, save, rend);
}

static void clear_line(const uint8_t row)
{
	xcb_clear_area(jbxvt.X.xcb, 0, jbxvt.X.win.vt, 0, row * FSZ.h,
		PSZ.w, CSZ.w * FSZ.h);
}

void scroll1(const int16_t n)
{
	SLOG("scroll1(%d)", n);
	copy_saved_lines(n);
	jbxvt.scr.sline.top = MIN(jbxvt.scr.sline.top + n,
		jbxvt.scr.sline.max);
	for (int_fast16_t j = n;
		j < jbxvt.scr.chars.height; ++j)
		  move_line(j, -n, &jbxvt.scr.s[0]);
}


static void sc_dn(const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	for(int8_t j = copy_screen_area(0, row2, -1,
		count, save, rend); j >= row1; --j)
		  move_line(j, count, jbxvt.scr.current);
	clear(count, row1, save, rend, false);
	copy_visible_area(row1, row2, count, false);
	clear_line(row1);
}

static void sc_up(const uint8_t row1, const uint8_t row2,
	const int16_t count, uint8_t ** save, uint32_t ** rend)
{
	if (jbxvt.scr.current == &jbxvt.scr.s[0] && row1 == 0)
		add_scroll_history(count);
	for(int8_t j = copy_screen_area(0, row1,
		1, count, save, rend); j < row2; ++j)
		move_line(j, -count, jbxvt.scr.current);
	clear(count, row2, save, rend, true);
	copy_visible_area(row1, row2, count, true);
	clear_line(row2 - count);
}

/*  Scroll count lines from row1 to row2 inclusive.
    row1 should be <= row2.  Scrolling is up for
    a positive count and down for a negative count.
    count is limited to a maximum of JBXVT_MAX_SCROLL lines.  */
void scroll(const uint8_t row1, const uint8_t row2,
	const int16_t count)
{
	SLOG("scroll(%d, %d, %d)", row1, row2, count);
	// Sanitize input:
	if(!count || row1 > row2
		|| row2 >= jbxvt.scr.chars.height
		|| abs(count) > JBXVT_MAX_SCROLL)
		  return;
	const bool up = count > 0;
	const int16_t n = up ? count : -count; // make positive
	uint8_t *save[n];
	uint32_t *rend[n];
	// row2 + 1 to include last line
	(up ? sc_up : sc_dn)(row1, row2 + 1, n, save, rend);
	change_offset(0);
	draw_cursor(); // clear
	draw_cursor();
}

