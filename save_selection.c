/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "save_selection.h"
#include <stdlib.h>
#include <string.h>
#include "JBXVTLine.h"
#include "JBXVTSelectionData.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "screen.h"
#include "selend.h"
#include "size.h"
static void copy(uint8_t * str, uint8_t * scr_text,
	const int16_t start, const uint16_t len,
	const uint16_t total)
{
	if (!len)
		return; // prevent segmentation fault on empty data.
	char * dest = (char *) str + total - 1;
	char * src = (char *) scr_text + start;
	strncpy(dest, src, len + 1);
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
	uint8_t * restrict s = *str;
	bool last_was_cr = false;
	for (int_fast16_t i = 0; i < *total; ++i)
		if (s[i] == '\0') {
			if (!last_was_cr) {
				s[i] = '\r';
				last_was_cr = true;
			}
		} else
			last_was_cr = false;
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
