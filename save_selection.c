/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "save_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "selend.h"

#include <gc.h>
#include <stdlib.h>
#include <string.h>

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
	if (str[++*i] != '\0')
		return false;
	if (*i >= len - 1) {
		str[*j] = '\0';
		if (null_ct == 1)
			--*j;
		return true;
	}
	return skip(str, i, j, len, null_ct);
}

static uint16_t sanitize(uint8_t * str, const uint16_t len)
{
	uint16_t i, j;
	uint8_t null_ct = 0;
	for (i = 0, j = 0; i < len; ++i, ++j) {
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

static void handle_screensel(uint8_t ** str, uint16_t * restrict total,
	SelEnd * restrict e)
{
	if (e->index == (e+1)->index && e->col == (e+1)->col)
		return; // NULL selection
	for (int_fast16_t i = e->index, j = (e+1)->index; i <= j; ++i) {
		/* Use full screen width if not first or last lines of
		   selection, otherwise use the col field in the respective
		   end point.  */
		const uint16_t w = jbxvt.scr.chars.width;
		const int16_t start = i == e->index ? e->col : 0;
		const int16_t end = i == j ? (e+1)->col : w - 1;
		const uint16_t len = end - start + 1;
		*str = GC_REALLOC(*str, *total + len);
		strncpy((char*)*str + *total,
			(char*)jbxvt.scr.current->text[i] + start, len);
		*total += len;
	}
	*total = sanitize(*str, *total) + 1;
	*str = GC_REALLOC(*str, *total);
}

/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection. */
void save_selection(void)
{
	/*  Set se1 and se2 to point to the first
	    and second selection endpoints.  */
	struct JBXVTSelectionData * s = &jbxvt.sel;
	SelEnd * e = s->end;
	const bool forward = selcmp(e, e+1) <= 0;
	SelEnd se[] = {e[forward?0:1], e[forward?1:0]};
	uint16_t total = 1;
	uint8_t * str = GC_MALLOC(total);
	handle_screensel(&str, &total, se);
	str[--total] = 0; // null termination
	s->text = str;
	s->length = total;
}

