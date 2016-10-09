/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/

#include "selend.h"

#include "jbxvt.h"


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
	if (jbxvt.sel.type == JBXVT_SEL_SAVED)
		  return cmp(1, se1, se2);
	if (jbxvt.sel.type == JBXVT_SEL_ON_SCREEN)
		  return cmp(-1, se1, se2);
	return 1;
}

//  Convert a row and column coordinates into a selection endpoint.
void jbxvt_rc_to_selend(const int16_t row, const int16_t col, struct JBDim * se)
{
	int16_t i = (row - jbxvt.scr.offset);
	if (i < 0) {
		i = -i;
		jbxvt.sel.type = JBXVT_SEL_SAVED;
	} else
		jbxvt.sel.type = JBXVT_SEL_ON_SCREEN;
	se->index = i;
	se->col = col;
}

//  Convert the selection into a row and column.
void jbxvt_selend_to_rc(int16_t * restrict rowp, int16_t * restrict colp,
	struct JBDim * restrict se)
{
	if (jbxvt.sel.type == JBXVT_SEL_NONE)
		return;

	*colp = se->col;
	*rowp = jbxvt.sel.type == JBXVT_SEL_ON_SCREEN
		? se->index + jbxvt.scr.offset
		: jbxvt.scr.offset - se->index;
}

static uint16_t sel_s(struct JBDim * restrict se2, uint8_t ** s)
{
	const bool ss = jbxvt.sel.type == JBXVT_SEL_ON_SCREEN;
#define SLD jbxvt.scr.sline.data
	*s = ss ? jbxvt.scr.current->text[se2->index]
		: SLD[se2->index].text;
	return ss ? jbxvt.scr.chars.width
		: SLD[se2->index].sl_length;
}

static void adj_sel_to_word(struct JBDim * include,
	struct JBDim * se1, struct JBDim * se2)
{
	uint8_t * s = jbxvt.sel.type == JBXVT_SEL_ON_SCREEN
		? jbxvt.scr.current->text[se1->index] : SLD[se1->index].text;
#undef SLD
	int16_t i = se1->col;
	while (i && s[i] != ' ')
		  --i;
	se1->col = i?i+1:0;
	i = se2->col;
	if (se2 == include || !jbxvt_selcmp(se2, &jbxvt.sel.end[2]))
		  ++i;
	const uint16_t len = sel_s(se2, &s);
	while (i < len && s[i] && s[i] != ' ' && s[i] != '\n')
		  ++i;
	se2->col = i;

}

/*  Adjust the selection to a word or line boundary.
    If the include endpoint is non NULL then the selection
    is forced to be large enough to include it.  */
void jbxvt_adjust_selection(struct JBDim * restrict include)
{
	if (jbxvt.sel.unit == JBXVT_SEL_UNIT_CHAR)
		return;
#define SE jbxvt.sel.end
	const bool oneless = jbxvt_selcmp(SE, SE + 1) <= 0;
	struct JBDim *se1, *se2;
	*(oneless ? &se1 : &se2) = SE;
	*(oneless ? &se2 : &se1) = SE + 1;
#undef SE
	if (jbxvt.sel.unit == JBXVT_SEL_UNIT_WORD)
		  adj_sel_to_word(include, se1, se2);
	else if (jbxvt.sel.unit == JBXVT_SEL_UNIT_LINE) {
		se1->col = 0;
		se2->col = jbxvt.scr.chars.width;
	}
}


