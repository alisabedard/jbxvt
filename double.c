// Copyright 2017, Jeffrey E. Bedard
#include "double.h"
#include <stdlib.h>
#include <string.h>
#include "cursor.h"
#include "repaint.h"
void jbxvt_set_double_width_line(xcb_connection_t * xc, const bool is_dwl)
{
	jbxvt_get_line(jbxvt_get_y())->dwl = is_dwl;
	jbxvt_repaint(xc); // in case set mid-line
	jbxvt_draw_cursor(xc); // clear stale cursor block
}
/* Disperse each character of the input string to every other character of the
 * output string.  */
static void alt(const int i, const int in_length,
	uint8_t * restrict out_str,
	uint8_t * restrict in_str)
{
	if (i < in_length) {
		out_str[0] = in_str[i];
		alt(i + 1, in_length, out_str + 2, in_str);
	}
}
/* Initialize every other character of the output string
   to a space character. */
static void space(const int i, uint8_t * restrict out_str)
{
	if (i > 0) {
		if (i % 2)
			out_str[i] = ' ';
		space(i - 1, out_str);
	}
}
// Generate a double-width string.  Free the result!
uint8_t * jbxvt_get_double_width_string(uint8_t * in_str, uint16_t * restrict
	length_return)
{
	// save current length
	const int in_length = *length_return, out_length = in_length << 1;
	*length_return = out_length;
	// double it and allocate buffer to match
	uint8_t * out_str = malloc(out_length);
	space(out_length - 1, out_str);
	alt(0, in_length, out_str, in_str);
	return out_str;
}
