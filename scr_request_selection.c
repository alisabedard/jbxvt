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
	for (uint16_t i = 0; i < count; i++)
		if (str[i] == '\n')
			str[i] = '\r';
	send_string(str,count);
}

static void use_cut_buffer(void)
{
	unsigned long nread = 0, bytes_after;
	do {
		uint8_t * data;
		// divide by 4 to convert 32 bit words to bytes
		xcb_get_property_cookie_t c = xcb_get_property(jbxvt.X.xcb,
			false, jbxvt.X.screen->root, XCB_ATOM_CUT_BUFFER0,
			XCB_ATOM_STRING, nread / 4, PROP_SIZE);
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

//  Request the current primary selection
void scr_request_selection(xcb_timestamp_t time, const xcb_point_t p)
{
	xcb_intern_atom_cookie_t c = xcb_intern_atom(jbxvt.X.xcb, false,
		12, "VT_SELECTION");
	xcb_get_selection_owner_cookie_t owner_c = xcb_get_selection_owner(
		jbxvt.X.xcb, XCB_ATOM_PRIMARY);

	//  First check that the release is within the window.
	if (p.x < 0 || p.x >= jbxvt.scr.pixels.width || p.y < 0
		|| p.y >= jbxvt.scr.pixels.height)
		  return;

	if (jbxvt.sel.text) { // the selection is internal
		send_selection(jbxvt.sel.text,jbxvt.sel.length);
		return;
	}
	xcb_get_selection_owner_reply_t * owner_r
		= xcb_get_selection_owner_reply(jbxvt.X.xcb, owner_c, NULL);
	if(!owner_r->owner) {
		//  No primary selection so use the cut buffer.
		use_cut_buffer();
		free(owner_r);
		return;
	}
	free(owner_r);
	xcb_intern_atom_reply_t * r = xcb_intern_atom_reply(jbxvt.X.xcb,
		c, NULL);
	xcb_convert_selection(jbxvt.X.xcb, jbxvt.X.win.vt, XCB_ATOM_PRIMARY,
		XCB_ATOM_STRING, r->atom, time);
	free(r);
}

//  Respond to a notification that a primary selection has been sent
void scr_paste_primary(const xcb_window_t window, const xcb_atom_t property)
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


