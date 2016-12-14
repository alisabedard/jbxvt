/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "save_selection.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "JBXVTLine.h"
#include "JBXVTSelectionData.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "screen.h"
#include "selend.h"
#include "size.h"
static uint8_t get_next_char(uint8_t c)
{
	if (c == '\n')
		return '\r';
	else if (c < ' ' || c > 0x7e)
		return ' ';
	return c;
}
static bool skip(uint8_t * restrict str, uint16_t * restrict i,
	uint16_t * restrict j, const uint16_t len, const uint8_t null_ct)
{
	++*i;
	if (str[*i] != '\0')
		return false;
	if (*i >= len - 1)
		return true;
	return skip(str, i, j, len, null_ct);
}
static uint16_t sanitize(uint8_t * str, uint16_t len)
{
	uint16_t i = 0, j;
	uint8_t null_ct = 0;
	/* This conditional fixes insertion of rogue new lines
	   before selection text.  This makes copying filenames
	   and keys easier on the command line.  */
	if (i < len && str[i] == '\0') {
		++i;
		--len;
	}
	for (j = 0; i < len; ++i, ++j) {
		if (str[i] == '\0') {
			++null_ct;
			str[j] = '\r';
			if (skip(str, &i, &j, len, null_ct))
				return j;
			--i;
		} else
			str[j] = get_next_char(str[i]);
	}
	str[j] = '\0';
	return j;
}
static void copy(uint8_t * str, uint8_t * scr_text,
	const int16_t start, const uint16_t len,
	const uint16_t total)
{
	char * dest = (char *) str + total - 1;
	char * src = (char *) scr_text + start;
	strncpy(dest, src, len);
}
static inline bool is_on_screen(const struct JBDim p)
{
	const struct JBDim c = jbxvt_get_char_size();
	return p.x < c.x && p.y < c.y;
}
static void limit_anchor(struct JBDim * anchor)
{
	const struct JBDim c = jbxvt_get_char_size();
	JB_LIMIT(anchor->x, c.x - 1, 0);
	JB_LIMIT(anchor->y, c.y - 1, 0);
}
static void handle_screensel(uint8_t ** str, uint16_t * restrict total,
	struct JBDim * restrict e)
{
	// Make sure the selection falls within the screen area:
	if (!is_on_screen(e[0]) || !is_on_screen(e[1]))
		return; // Invalid end point found
	limit_anchor(e + 2);
	for (int_fast16_t i = e[0].index, j = e[1].index; i <= j; ++i) {
		/* Use full screen width if not first or last lines of
		   selection, otherwise use the col field in the respective
		   end point.  */
		const int16_t start = i == e->index ? e->col : 0;
		const int16_t end = i == j ? (e+1)->col
			: jbxvt_get_char_size().width - 1;
		const uint16_t len = end - start;
		*str = realloc(*str, *total + len);
		copy(*str, jbxvt_get_line(i)->text, start, len, *total);
		*total += len;
	}
	*total = sanitize(*str, *total) + 1;
	*str = realloc(*str, *total);
}
/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection. */
void jbxvt_save_selection(struct JBXVTSelectionData * sel)
{
	const bool fwd = jbxvt_selcmp(sel->end, sel->end + 1) <= 0;
	// properly order start and end points:
	struct JBDim se[] = {sel->end[fwd ? 0 : 1],
		sel->end[fwd ? 1 : 0]};
	uint16_t total = 1;
	uint8_t * str = malloc(total);
	handle_screensel(&str, &total, se);
	if (!total)
		return;
	str[sel->length = --total] = 0; // null termination
	sel->text = str;
	LOG("jbxvt_save_selection() sel->text: %s, sel->unit: %d, "
		"sel->length: %d", sel->text, sel->unit, sel->length);
}
