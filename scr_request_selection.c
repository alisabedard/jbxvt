/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_request_selection.h"

#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "selection.h"

#include <stdlib.h>

//  Send the selection to the command after converting LF to CR.
static void send_selection(uint8_t * str, const uint16_t count)
{
	for (uint16_t i = count; i >= 0; --i)
		if (str[i] == '\n')
			str[i] = '\r';
	send_string(str, count);
}

void request_selection(void)
{
	xcb_get_property_cookie_t c = xcb_get_property(jbxvt.X.xcb,
		false, jbxvt.X.screen->root, XCB_ATOM_CUT_BUFFER0,
		XCB_ATOM_STRING, 0, PROP_SIZE);
	xcb_get_property_reply_t * r = xcb_get_property_reply(
		jbxvt.X.xcb, c, NULL);
	if (!r)
		  return;
	send_selection(xcb_get_property_value(r),
		xcb_get_property_value_length(r));
	free(r);
}

//  Respond to a notification that a primary selection has been sent
void paste_primary(const xcb_window_t window, const xcb_atom_t property)
{
	if (property == XCB_NONE)
		return;
	uint32_t nread = 0, bytes_after;
	do {
		uint8_t * data;
		// divide by 4 to convert 32 bit words to bytes
		xcb_get_property_cookie_t c = xcb_get_property(jbxvt.X.xcb,
			false, window, property, XCB_ATOM_STRING, nread / 4,
			PROP_SIZE);
		xcb_get_property_reply_t * r = xcb_get_property_reply(
			jbxvt.X.xcb, c, NULL);
		if (!r)
			  return;
		data = xcb_get_property_value(r);
		nread += xcb_get_property_value_length(r);
		bytes_after = r->bytes_after;
		send_selection(data, xcb_get_property_value_length(r));
		free(r);
	} while (bytes_after > 0);
}


