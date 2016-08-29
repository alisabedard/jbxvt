/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_edit.h"

#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"

#include <string.h>

#define FSZ jbxvt.X.f.size
#define SCR jbxvt.scr.current
#define XC jbxvt.X.xcb
#define VT jbxvt.X.win.vt
#define CSZ jbxvt.scr.chars

static void copy_area(const int16_t * restrict x, const int16_t y,
	const uint16_t width)
{
	if (width > 0)
		xcb_copy_area(XC, VT, VT, jbxvt.X.gc.tx, x[0], y,
			x[1], y, width, FSZ.height);
}

static void finalize(const struct JBDim p, const int8_t count)
{
	xcb_clear_area(XC, 0, VT, p.x, p.y, count * FSZ.w, FSZ.h);
	SCR->wrap_next = 0;
	draw_cursor();
}

static void copy_lines(const int16_t x, const int8_t count)
{
	const int16_t y = SCR->cursor.y;
	uint8_t * s = SCR->text[y];
	uint32_t * r = SCR->rend[y];
	for (int16_t i = CSZ.w - 1; i >= x + count; --i) {
		s[i] = s[i - count];
		r[i] = r[i - count];
	}
}

//  Insert count spaces from the current position.
void scr_insert_characters(int8_t count)
{
	LOG("scr_insert_characters(%d)", count);
	count = MAX(count, 0);
	count = MIN(count, CSZ.w);
	change_offset(0);
	draw_cursor();
	const struct JBDim c = SCR->cursor;
	check_selection(c.y, c.y);
	copy_lines(c.x, count);
	const struct JBDim p = get_p(c);
	const uint16_t width = (CSZ.w - count - c.x) * FSZ.width;
	copy_area((int16_t[]){p.x, p.x + count * FSZ.width}, p.y, width);
	finalize(p, count);
}

static void copy_data_after_count(const uint8_t count, const struct JBDim c)
{
	// copy the data after count
	const uint16_t offset = c.x + count;
	const uint16_t end = CSZ.width - c.x;
	uint8_t * t = SCR->text[c.y];
	uint32_t * r = SCR->rend[c.y];
	memmove(t + c.x, t + offset, end - count);
	memmove(r + c.x, r + offset,
		(end - count) * sizeof(uint32_t));
}

static void delete_source_data(const uint8_t count, const int16_t y)
{
	const uint16_t scw = jbxvt.scr.chars.width;
	VTScreen * scr = jbxvt.scr.current;
	uint8_t * t = scr->text[y];
	uint32_t * r = scr->rend[y];
	// delete the source data copied
	memset(t + scw - count, 0, count);
	memset(r + scw - count, 0, count << 2);
}

//  Delete count characters from the current position.
void scr_delete_characters(uint8_t count)
{
	LOG("scr_delete_characters(%d)", count);
	const struct JBDim c = SCR->cursor;
	const uint8_t end = CSZ.width - c.x;
	count = MIN(count, end); // keep within the screen
	if(!count)
		  return;
	change_offset(0);
	draw_cursor();
	copy_data_after_count(count, c);
	delete_source_data(count, c.y);
	struct JBDim p = get_p(c);
	int16_t x[] = {p.x + count * FSZ.w, p.x};
	const uint16_t width = (end - count) * FSZ.w;
	copy_area(x, p.y, width);
	p.x += width;
	finalize(p, count);
}

