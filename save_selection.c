/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "save_selection.h"
#include <stdlib.h>
#include <string.h>
#include "JBXVTLine.h"
#include "JBXVTSelectionData.h"
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
__attribute__((const))
static inline bool end_point_on_screen(const struct JBDim pos,
	const struct JBDim char_sz)
{
	return pos.col < char_sz.col && pos.row < char_sz.row;
}
__attribute__((pure))
static bool is_not_on_screen(const struct JBDim * restrict e)
{
	const struct JBDim c = jbxvt_get_char_size();
	return !end_point_on_screen(e[0], c)
		|| !end_point_on_screen(e[1], c);
}
/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection. */
void jbxvt_save_selection(struct JBXVTSelectionData * sel)
{
	const bool fwd = jbxvt_selcmp(&sel->end[0], &sel->end[1]) <= 0;
	// properly order start and end points:
	struct JBDim e[] = {sel->end[fwd ? 0 : 1],
		sel->end[fwd ? 1 : 0]};
	uint16_t total = 1;
	uint8_t * str = malloc(total);
	// Make sure the selection falls within the screen area:
	if (is_not_on_screen(e))
		return; // Invalid end point found
	// Copy each line in the selection:
	for (int_fast16_t i = e[0].index, j = e[1].index; i <= j; ++i) {
		/* Use full screen width if not first or last lines of
		   selection, otherwise use the col field in the respective
		   end point.  */
		const int16_t start = i == e[0].index ? e[0].col : 0;
		const int16_t end = i == j ? e[1].col
			: jbxvt_get_char_size().width - 1;
		const uint16_t len = end - start;
		str = realloc(str, total + len);
		copy(str, jbxvt_get_line(i)->text, start, len, total);
		total += len;
	}
	bool last_was_cr = false;
	// Substitute the first null terminator with a carriage return:
	for (int_fast16_t i = 0; i < total; ++i)
		if (str[i] == '\0') {
			if (!last_was_cr) {
				str[i] = '\r';
				last_was_cr = true;
			}
		} else
			last_was_cr = false;
	sel->text = str = realloc(str, total);
	if (str && total > 0) // avoid segmentation fault if total is invalid
		str[sel->length = --total] = 0; // null termination
}
