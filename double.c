// Copyright 2016, Jeffrey E. Bedard
#include "double.h"
#include <stdlib.h>
#include "JBXVTScreen.h"
#include "cursor.h"
#include "repaint.h"
#include "screen.h"
void jbxvt_set_double_width_line(xcb_connection_t * xc, const bool is_dwl)
{
	jbxvt_get_line(jbxvt_get_y())->dwl = is_dwl;
	jbxvt_repaint(xc); // in case set mid-line
	jbxvt_draw_cursor(xc); // clear stale cursor block
}
// Generate a double-width string.  Free the result!
uint8_t * jbxvt_get_double_width_string(uint8_t * in_str, uint16_t * len)
{
	// save current length
	const uint16_t l = *len;
	// double it and allocate buffer to match
	uint8_t * o = malloc(*len <<= 1);
	{ // * j scope
		uint8_t * j = o;
		for (uint_fast16_t i = 0; i < l; ++i, j += 2) {
			j[0] = in_str[i];
			j[1] = ' ';
		}
	}
	return o;
}
