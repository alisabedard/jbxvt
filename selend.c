/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "selend.h"
#include "jbxvt.h"
#include "sbar.h"
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
int8_t jbxvt_selcmp(struct JBDim * restrict se1, struct JBDim * restrict se2)
{
	if (jbxvt.sel.type == JBXVT_SEL_ON_SCREEN)
		  return cmp(-1, se1, se2);
	return 1;
}
//  Convert a row and column coordinates into a selection endpoint.
void jbxvt_rc_to_selend(const int16_t row, const int16_t col,
	struct JBDim * se)
{
	jbxvt.sel.type = JBXVT_SEL_ON_SCREEN;
	se->index = row - jbxvt_get_scroll();
	se->col = col;
}
//  Convert the selection into a row and column.
void jbxvt_selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct JBDim * restrict se)
{
	if (jbxvt.sel.type == JBXVT_SEL_NONE)
		return;
	*colp = se->col;
	*rowp = se->row + jbxvt_get_scroll();
}
static uint16_t sel_s(struct JBDim * restrict se2, uint8_t ** s)
{
	*s = jbxvt.scr.current->text[se2->index];
	return jbxvt.scr.chars.width;
}
static int16_t get_start_of_word(uint8_t * restrict s, int16_t i)
{
	while (i && s[i] != ' ')
		  --i;
	return i;
}
static void adj_sel_to_word(struct JBDim * include,
	struct JBDim * se1, struct JBDim * se2)
{
	if (se1->index < 0)
		return; // protect against segfault if ends invalid
	uint8_t * s = jbxvt.scr.current->text[se1->index];
	int16_t i = get_start_of_word(s, se1->col);
	se1->col = i?i+1:0;
	i = se2->col;
	if (se2 == include || !jbxvt_selcmp(se2, &jbxvt.sel.end[2]))
		  ++i;
	const uint16_t len = sel_s(se2, &s);
	while (i < len && s[i] && s[i] != ' ' && s[i] != '\n')
		  ++i;
	se2->col = i;
}
// Make sure selection end point 0 comes before end point 1
void jbxvt_order_selection_ends(void)
{
	if (jbxvt_selcmp(jbxvt.sel.end, jbxvt.sel.end + 1) <= 0) {
		// copy data, not addresses, here
		const struct JBDim tmp = jbxvt.sel.end[0];
		jbxvt.sel.end[0] = jbxvt.sel.end[1];
		jbxvt.sel.end[1] = tmp;
	}
}
/*  Adjust the selection to a word or line boundary.
    If the include endpoint is non NULL then the selection
    is forced to be large enough to include it.  */
void jbxvt_adjust_selection(struct JBDim * restrict include)
{
	if (jbxvt.sel.unit == JBXVT_SEL_UNIT_CHAR)
		return;
	jbxvt_order_selection_ends();
	if (jbxvt.sel.unit == JBXVT_SEL_UNIT_WORD)
		  adj_sel_to_word(include, jbxvt.sel.end, jbxvt.sel.end + 1);
	else if (jbxvt.sel.unit == JBXVT_SEL_UNIT_LINE) {
		jbxvt.sel.end[0].col = 0;
		jbxvt.sel.end[1].col = jbxvt.scr.chars.width - 1;
	}
}
