/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "save_selection.h"

#include "config.h"
#include "jbxvt.h"
#include "selend.h"

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


#if defined(__i386__) || defined(__amd64__)
	__attribute__((pure,regparm(3)))
#else
	__attribute__((pure))
#endif
static inline uint8_t compute_i2(const uint16_t len, const int16_t i1,
	int16_t i2, uint8_t * restrict str)
{
	i2 = MIN(i2, len - 1);
	if (i2 - i1 >= JBXVT_MAX_COLS)
		i2 = i1 + JBXVT_MAX_COLS;
	while (i2 >= i1 && str[i2] == 0)
		--i2;
	return i2;
}

/*  Convert a section of displayed text line into a text string suitable
    for pasting. *lenp is the length of the input string, i1 is index
    of the first character to convert and i2 is the last.  The length
    of the returned string is returned in *lenp; */
static uint8_t * convert_line(uint8_t * restrict str,
	uint16_t * restrict lenp, int16_t i1, int16_t i2)
{
	// set this before i2 is modified
	const bool newline = (i2 >= jbxvt.scr.chars.width)
		&& (str[*lenp] == 0);
	i2 = compute_i2(*lenp, i1, i2, str);
	uint8_t * buf = GC_MALLOC(JBXVT_PROP_SIZE);
	uint8_t * s = buf;
	for (; i1 <= i2; ++i1, ++s)
		if (str[i1] >= ' ')
			*s = str[i1];
		else if (str[i1] == '\n')
			*s = '\r';
		else
			*s = ' ';
	if (newline)
		*s++ = '\r';
	*s = 0; // NULL termination
	*lenp = s - buf;
	return (buf);
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
		uint16_t len = jbxvt.scr.chars.width;
		int16_t col2 = i == se2->index ? se2->col : len - 1;
		if (--col2 < 0)
			  break;
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
	const bool forward = selcmp(&jbxvt.sel.end[0], &jbxvt.sel.end[1]) <= 0;
	SelEnd * se1 = forward ? &jbxvt.sel.end[0] : &jbxvt.sel.end[1];
	SelEnd * se2 = forward ? &jbxvt.sel.end[1] : &jbxvt.sel.end[0];
	uint16_t total = 1;
	uint8_t * str = GC_MALLOC(1);
	handle_savedsel(&str, &total, se1, se2);
	handle_screensel(&str, &total, se1, se2);
	str[total - 1] = 0; // null termination
	jbxvt.sel.text = str;
	jbxvt.sel.length = total - 1;
}

