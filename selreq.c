/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "selreq.h"
#include "command.h"
#include "config.h"
#include "libjb/log.h"
#include "selection.h"
#include "window.h"
#include <stdlib.h>
#include <unistd.h>
static inline void paste(const uint8_t * data, const size_t length)
{
	jb_check(write(jbxvt_get_fd(), data, length) != -1,
		"Cannot paste");
}
static bool reply_is_invalid(xcb_get_property_reply_t * restrict r)
{
	if (!r) // no reply received, invalid
		return true;
	if (r->type == XCB_ATOM_NONE) {
		free(r);
		return true; // no data in reply, invalid
	}
	return false; // reply is valid
}
static bool paste_from(xcb_connection_t * xc,
	const xcb_atom_t cb, const xcb_timestamp_t t)
{
	LOG("paste_from(clipboard: %d, timestamp: %d)", cb, t);
	xcb_convert_selection(xc, jbxvt_get_main_window(xc), cb,
		XCB_ATOM_STRING, cb, t);
	xcb_flush(xc);
	// This prevents pasting stale data:
	free(xcb_wait_for_event(xc)); // discard
	xcb_get_property_reply_t * r = xcb_get_property_reply(xc,
		xcb_get_property(xc, false, jbxvt_get_main_window(xc),
		cb, XCB_ATOM_ANY, 0, JBXVT_PROP_SIZE), NULL);
	if (reply_is_invalid(r))
		return false;
	paste(xcb_get_property_value(r),
		xcb_get_property_value_length(r));
	free(r);
	return true;
}
void jbxvt_request_selection(xcb_connection_t * xc,
	const xcb_timestamp_t t)
{
	LOG("jbxvt_request_selection(timestamp: %d)", t);
	if (paste_from(xc, XCB_ATOM_PRIMARY, t))
		  return;
	if (paste_from(xc, jbxvt_get_clipboard(xc), t))
		  return;
	if (paste_from(xc, XCB_ATOM_SECONDARY, t))
		  return;
}
//  Respond to a notification that a primary selection has been sent
void jbxvt_paste_primary(xcb_connection_t * xc,
	const xcb_timestamp_t t, const xcb_window_t window,
	const xcb_atom_t property)
{
	LOG("paste_primary()");
	if (property == XCB_NONE)
		return;
	uint32_t nread = 0, bytes_after;
	xcb_convert_selection(xc, window, property,
		XCB_ATOM_STRING, property, t);
	do {
		uint8_t * data;
		// divide by 4 to convert 32 bit words to bytes
		xcb_get_property_cookie_t c = xcb_get_property(xc,
			false, window, property, XCB_ATOM_ANY, nread / 4,
			JBXVT_PROP_SIZE);
		xcb_get_property_reply_t * r
			= xcb_get_property_reply(xc, c, NULL);
		if (reply_is_invalid(r))
			return;
		data = xcb_get_property_value(r);
		const int l = xcb_get_property_value_length(r);
		nread += l;
		bytes_after = r->bytes_after;
		paste(data, l);
		free(r);
	} while (bytes_after > 0);
}
