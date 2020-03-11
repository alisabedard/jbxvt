/*  Copyright 2017, Jeffrey E. Bedard
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
/*  Compare the two selections and return negtive,
    0 or positive depending on whether se2 is after,
    equal to or before se1.  */
int8_t jbxvt_selcmp(struct JBDim * se1,
    struct JBDim * se2)
{
    if (!jbxvt_is_selected())
        return 0;
    if (se1->index == se2->index)
        return se1->col - se2->col;
    return se1->index - se2->index;
}
//  Convert a row and column coordinates into a selection endpoint.
void jbxvt_rc_to_selend(const int16_t row, const int16_t col,
    struct JBDim * se)
{
    se->index = row - jbxvt_get_scroll();
    se->col = col;
}
//  Convert the selection into a row and column.
void jbxvt_selend_to_rc(int16_t * rowp, int16_t * colp,
    struct JBDim * se)
{
    if (jbxvt_is_selected()) {
        *colp = se->col;
        *rowp = se->row + jbxvt_get_scroll();
    }
}
// Convert the selection end point into a dimension structure
struct JBDim jbxvt_get_selend_position(struct JBDim * se)
{
    return (struct JBDim){.row = se->row + jbxvt_get_scroll(), .col =
        se->col};
}
static inline uint8_t * get_text_at(struct JBDim * endpoint)
{
    return jbxvt_get_line(endpoint->index)->text;
}
static int16_t get_start_of_word(uint8_t * s, int16_t i)
{
    return i && s[i] > ' ' ? get_start_of_word(s, i - 1) : i;
}
static int find_word_end(const int i, const int len,
    uint8_t * text)
{
    return (i < len && text[i]  > ' ')
        ? find_word_end(i + 1, len, text) : i;
}
static void adjust_to_word(struct JBDim * include,
    struct JBDim * se1, struct JBDim * se2)
{
    if (se1->index < 0)
        return; // protect against segfault if end is invalid
    uint8_t * text = get_text_at(se1);
    int i;
    se1->col = (i = get_start_of_word(text, se1->col))
        ? i + 1 : 0;
    i = se2->col;
    if (se2 == include || !jbxvt_selcmp(se2,
        &jbxvt_get_selection_end_points()[2]))
          ++i;
    text = get_text_at(se2);
    se2->col = find_word_end(i, jbxvt_get_char_size().width, text);
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
void jbxvt_adjust_selection(struct JBDim * include)
{
    const enum JBXVTSelectionUnit u = jbxvt_get_selection_unit();
    if (u == JBXVT_SEL_UNIT_CHAR)
        return;
    struct JBDim * e = jbxvt_order_selection_ends(
        jbxvt_get_selection_end_points());
    if (include && u == JBXVT_SEL_UNIT_WORD)
          adjust_to_word(include, e, e + 1);
    else if (u == JBXVT_SEL_UNIT_LINE) {
        e[0].col = 0;
        e[1].col = jbxvt_get_char_size().width - 1;
    }
}
