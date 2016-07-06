/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scr_request_selection.h"

#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "selection.h"

#include <stdlib.h>
#include <string.h>

//  Send the selection to the command after converting LF to CR.
static void send_selection(uint8_t * str, const uint16_t count)
{
	LOG("send_selection(str, count: %d)", count);
	if (!count)
		  return;
	for (char * i = (char *)str; (i = strchr(i, '\n')); *i = '\r')
		  ;
	send_string(str, count);
}

void request_selection(void)
{
	LOG("request_selection");
	xcb_get_selection_owner_cookie_t o_c
		= xcb_get_selection_owner_unchecked(jbxvt.X.xcb,
			XCB_ATOM_PRIMARY);
	xcb_get_selection_owner_reply_t * o_r
		= xcb_get_selection_owner_reply(jbxvt.X.xcb, o_c, NULL);
	if (!o_r) {
		LOG("could not get owner");
		return;
	}
	xcb_get_property_cookie_t p_c = xcb_get_property(jbxvt.X.xcb,
		false, o_r->owner, XCB_ATOM_PRIMARY, XCB_ATOM_STRING,
		0, PROP_SIZE);
	free(o_r);
	xcb_get_property_reply_t * p_r = xcb_get_property_reply(jbxvt.X.xcb,
		p_c, NULL);
	if (!p_r) {
		LOG("could not get property");
		return;
	}
#ifdef DEBUG
	xcb_get_atom_name_cookie_t a_c = xcb_get_atom_name(jbxvt.X.xcb,
		p_r->type);
	xcb_get_atom_name_reply_t * a_r = xcb_get_atom_name_reply(jbxvt.X.xcb,
		a_c, NULL);
	LOG("value: %d -- %s", p_r->type, xcb_get_atom_name_name(a_r));
	free(a_r);
#endif//DEBUG
	send_selection(xcb_get_property_value(p_r),
		xcb_get_property_value_length(p_r));
	free(p_r);
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

