/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "selend.h"
#include "JBXVTLine.h"
#include "JBXVTSelectionUnit.h"
#include "libjb/JBDim.h"
#include "sbar.h"
#include "screen.h"
#include "selection.h"
#include "size.h"
static int8_t cmp(const int8_t mod, struct JBDim * restrict se1,
	struct JBDim * restrict se2)
{
	if (se1->index == se2->index)
		return se1->col - se2->col;
	return (se2->index - se1->index) * mod;
}
/*  Compare the two selections and return negtive,
    0 or positive depending on whether se2 is after,
    equal to or before se1.  */
int8_t jbxvt_selcmp(struct JBDim * restrict se1,
	struct JBDim * restrict se2)
{
	if (jbxvt_is_selected())
		  return cmp(-1, se1, se2);
	return 1;
}
//  Convert a row and column coordinates into a selection endpoint.
void jbxvt_rc_to_selend(const int16_t row, const int16_t col,
	struct JBDim * se)
{
	se->index = row - jbxvt_get_scroll();
	se->col = col;
}
//  Convert the selection into a row and column.
void jbxvt_selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct JBDim * restrict se)
{
	if (jbxvt_is_selected()) {
		*colp = se->col;
		*rowp = se->row + jbxvt_get_scroll();
	}
}
static inline uint8_t * get_text_at(struct JBDim * endpoint)
{
	return jbxvt_get_line(endpoint->index)->text;
}
static int16_t get_start_of_word(uint8_t * restrict s, int16_t i)
{
	return i && s[i] > ' ' ? get_start_of_word(s, i - 1) : i;
}
static void adj_sel_to_word(struct JBDim * include,
	struct JBDim * se1, struct JBDim * se2)
{
	if (se1->index < 0)
		return; // protect against segfault if end is invalid
	int16_t i;
	{ // text scope
		uint8_t * text = get_text_at(se1);
		se1->col = (i = get_start_of_word(text, se1->col))
			? i + 1 : 0;
		i = se2->col;
		if (se2 == include || !jbxvt_selcmp(se2,
			&jbxvt_get_selection_end_points()[2]))
			  ++i;
		text = get_text_at(se2);
		for (const uint16_t len = jbxvt_get_char_size().width;
			i < len && text[i] > ' '; ++i)
			;
	}
	se2->col = i;
}
// Make sure selection end point 0 comes before end point 1
struct JBDim * jbxvt_order_selection_ends(struct JBDim * e)
{
	if (jbxvt_selcmp(e, e + 1) <= 0) {
		// copy data, not addresses, here
		const struct JBDim tmp = e[0];
		e[0] = e[1];
		e[1] = tmp;
	}
	return e;
}
/*  Adjust the selection to a word or line boundary.
    If the include endpoint is non NULL then the selection
    is forced to be large enough to include it.  */
void jbxvt_adjust_selection(struct JBDim * restrict include)
{
	const enum JBXVTSelectionUnit u = jbxvt_get_selection_unit();
	if (u == JBXVT_SEL_UNIT_CHAR)
		return;
	struct JBDim * e = jbxvt_order_selection_ends(
		jbxvt_get_selection_end_points());
	if (u == JBXVT_SEL_UNIT_WORD)
		  adj_sel_to_word(include, e, e + 1);
	else if (u == JBXVT_SEL_UNIT_LINE) {
		e[0].col = 0;
		e[1].col = jbxvt_get_char_size().width - 1;
	}
}
