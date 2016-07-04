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

static void realloc_and_copy(uint8_t ** str, uint8_t * restrict src,
	uint16_t * restrict total, const uint16_t len)
{
	*str = GC_REALLOC(*str, *total + len);
	strncpy((char *)*str + *total - 1, (char *)src, len);
	*total += len;
}

static void handle_savedsel(uint8_t ** str, uint16_t * restrict total,
	SelEnd * restrict se1, SelEnd * restrict se2)
{
	if (se1->type != SAVEDSEL)
		  return;
	int16_t col1 = se1->col;
	for (int_fast16_t i = se1->index; i >= 0; --i) {
		SLine * sl = jbxvt.scr.sline.data[i];
		int16_t col2;
		if (se2->type == SAVEDSEL && se2->index == i) {
			col2 = se2->col - 1;
			i = 0;	// force loop exit
		} else
			  col2 = jbxvt.scr.chars.width - 1;
		uint16_t len = sl->sl_length;
		uint8_t * s = convert_line(sl->sl_text,
			&len, col1, col2);
		realloc_and_copy(str, s, total, len);
		col1 = 0;
	}
}

static void handle_screensel(uint8_t ** str, uint16_t * restrict total,
	SelEnd * restrict se1, SelEnd * restrict se2)
{
	if (se2->type != SCREENSEL)
		  return;
	const bool is_screensel = se1->type == SCREENSEL;
	int_fast16_t i = is_screensel ? se1->index : 0;
	int16_t col1 = is_screensel ? se1->col : 0;
	for (; i <= se2->index; ++i) {
		int16_t col2 = i == se2->index ? se2->col
			: jbxvt.scr.chars.width - 1;
		if (--col2 < 0)
			  break;
		uint16_t len = jbxvt.scr.chars.width;
		uint8_t * s = convert_line(jbxvt.scr.current->text[i],
			&len, col1, col2);
		realloc_and_copy(str, s, total, len);
		col1 = 0;
	}
}

/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection. */
void save_selection(void)
{
	/*  Set se1 and se2 to point to the first
	    and second selection endpoints.  */
	const bool forward = selcmp(&jbxvt.sel.end1, &jbxvt.sel.end2) <= 0;
	SelEnd * se1 = forward ? &jbxvt.sel.end1 : &jbxvt.sel.end2;
	SelEnd * se2 = forward ? &jbxvt.sel.end2 : &jbxvt.sel.end1;
	uint16_t total = 1;
	uint8_t * str = GC_MALLOC(1);
	handle_savedsel(&str, &total, se1, se2);
	handle_screensel(&str, &total, se1, se2);
	str[total - 1] = 0; // null termination
	jbxvt.sel.text = str;
	jbxvt.sel.length = total - 1;
}


