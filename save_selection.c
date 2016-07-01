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

/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection.  true is returned
    for a success, false for a failure.  */
bool save_selection(void)
{
	uint8_t *str, *s;
	uint16_t len;
	int i, total, col1, col2;
	struct selst *se1, *se2;
	struct slinest *sl;

	if (jbxvt.sel.end1.se_type == NOSEL
		|| jbxvt.sel.end2.se_type == NOSEL)
		return false;
	if (jbxvt.sel.end1.se_type == jbxvt.sel.end2.se_type
		&& jbxvt.sel.end1.se_index == jbxvt.sel.end2.se_index
		&& jbxvt.sel.end1.se_col == jbxvt.sel.end2.se_col)
		return false;

	if (jbxvt.sel.text)
		GC_FREE(jbxvt.sel.text);

	/*  Set se1 and se2 to point to the first
	    and second selection endpoints.  */
	const bool forward = selcmp(&jbxvt.sel.end1, &jbxvt.sel.end2) <= 0;
	se1 = forward ? &jbxvt.sel.end1 : &jbxvt.sel.end2;
	se2 = forward ? &jbxvt.sel.end2 : &jbxvt.sel.end1;
	total = 1;
	str = GC_MALLOC(1);
	if (se1->se_type == SAVEDSEL) {
		col1 = se1->se_col;
		for (i = se1->se_index; i >= 0; i--) {
			sl = jbxvt.scr.sline.data[i];
			if (se2->se_type == SAVEDSEL && se2->se_index == i) {
				col2 = se2->se_col - 1;
				i = 0;			/* force loop exit */
			} else
				col2 = jbxvt.scr.chars.width - 1;
			len = sl->sl_length;
			s = convert_line(sl->sl_text,&len,col1,col2);
			str = GC_REALLOC(str,total + len);
			if (str == NULL)
				abort();
			strncpy((char *)str + total - 1,(char *)s,len);
			total += len;
			col1 = 0;
		}
	}
	if (se2->se_type == SCREENSEL) {
		const bool is_screensel = se1->se_type == SCREENSEL;
		i = is_screensel ? se1->se_index : 0;
		col1 = is_screensel ? se1->se_col : 0;
		for (; i <= se2->se_index; i++) {
			col2 = i == se2->se_index ? se2->se_col
				: jbxvt.scr.chars.width;
			if (--col2 < 0)
				break;
			len = jbxvt.scr.chars.width;
			s = convert_line(jbxvt.scr.current->text[i],&len,col1,col2);
			str = GC_REALLOC(str,total + len);
			if(!str)
				  abort();
			strncpy((char *)str + total - 1,(char *)s,len);
			total += len;
			col1 = 0;
		}
	}
	str[total - 1] = 0;
	jbxvt.sel.text = str;
	jbxvt.sel.length = total - 1;
	return true;
}


