/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "save_selection.h"

#include "jbxvt.h"
#include "screen.h"
#include "selcmp.h"
#include "selection.h"
#include "slinest.h"

#include <gc.h>
#include <stdlib.h>
#include <string.h>

static uint8_t * common(uint8_t * restrict str, uint8_t * restrict source,
	uint16_t * restrict total, int16_t * restrict col1, const uint16_t len)
{
	str = GC_REALLOC(str, *total + len);
	if(!str)
		  abort();
	strncpy((char *)str + *total - 1, (char *)source, len);
	total += len;
	return str;
	*col1 = 0;
}

/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection. */
void save_selection(void)
{
	uint8_t *str, *s;
	uint16_t len, total;
	int16_t i, col1, col2;
	struct selst *se1, *se2;
	struct slinest *sl;

	/*  Set se1 and se2 to point to the first
	    and second selection endpoints.  */
	const bool forward = selcmp(&jbxvt.sel.end1, &jbxvt.sel.end2) <= 0;
	se1 = forward ? &jbxvt.sel.end1 : &jbxvt.sel.end2;
	se2 = forward ? &jbxvt.sel.end2 : &jbxvt.sel.end1;
	total = 1;
	str = GC_MALLOC(1);
	const uint8_t w = jbxvt.scr.chars.width;
	if (se1->se_type == SAVEDSEL) {
		col1 = se1->se_col;
		for (i = se1->se_index; i >= 0; --i) {
			sl = jbxvt.scr.sline.data[i];
			if (se2->se_type == SAVEDSEL && se2->se_index == i) {
				col2 = se2->se_col - 1;
				i = 0;	// force loop exit
			} else
				col2 = w - 1;
			len = sl->sl_length;
			s = convert_line(sl->sl_text, &len, col1, col2);
			str = common(str, s, &total, &col1, len);
		}
	}
	if (se2->se_type == SCREENSEL) {
		const bool is_screensel = se1->se_type == SCREENSEL;
		i = is_screensel ? se1->se_index : 0;
		col1 = is_screensel ? se1->se_col : 0;
		for (; i <= se2->se_index; ++i) {
			col2 = i == se2->se_index ? se2->se_col : w;
			if (--col2 < 0)
				break;
			len = w;
			s = convert_line(jbxvt.scr.current->text[i], &len,
				col1, col2);
			common(str, s, &total, &col1, len);
		}
	}
	str[total - 1] = 0; // null termination
	jbxvt.sel.text = str;
	jbxvt.sel.length = total - 1;
}


