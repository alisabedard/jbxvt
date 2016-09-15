/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selreq.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define XC jbxvt.X.xcb
#define VT jbxvt.X.win.vt
#define COM jbxvt.com
#define PASTE(s, l) jb_check(write(COM.fd, s, l) != -1, "Cannot paste")

static bool paste_from(const xcb_atom_t cb, const xcb_timestamp_t t)
{
	xcb_convert_selection(XC, VT, cb, XCB_ATOM_STRING, cb, t);
	xcb_flush(XC);
	free(xcb_wait_for_event(XC)); // discard
	xcb_get_property_reply_t * r = xcb_get_property_reply(XC,
		xcb_get_property(XC, false, VT, cb, XCB_ATOM_ANY,
		0, JBXVT_PROP_SIZE), NULL);
	if (!r)
		return false;
	if (r->type == XCB_ATOM_NONE) {
		free(r);
		return false;
	}
	PASTE(xcb_get_property_value(r), xcb_get_property_value_length(r));
	free(r);
	return true;
}

void request_selection(const xcb_timestamp_t t)
{
	/* FIXME:  For some reason, the CLIPBOARD selection retrieved
	   first matches the old selection, then updates with a second
	   paste.  Why?  */
	if (paste_from(XCB_ATOM_PRIMARY, t))
		  return;
	if (paste_from(jbxvt.X.clipboard, t))
		  return;
	if (paste_from(XCB_ATOM_SECONDARY, t))
		  return;
}

//  Respond to a notification that a primary selection has been sent
void paste_primary(const xcb_timestamp_t t, const xcb_window_t window,
	const xcb_atom_t property)
{
	LOG("paste_primary()");
	if (property == XCB_NONE)
		return;
	uint32_t nread = 0, bytes_after;
	xcb_connection_t * restrict x = jbxvt.X.xcb;
	xcb_convert_selection(x, window, property,
		XCB_ATOM_STRING, property, t);
	do {
		uint8_t * data;
		// divide by 4 to convert 32 bit words to bytes
		xcb_get_property_cookie_t c = xcb_get_property(x,
			false, window, property, XCB_ATOM_ANY, nread / 4,
			JBXVT_PROP_SIZE);
		xcb_get_property_reply_t * r
			= xcb_get_property_reply(x, c, NULL);
		if (!r || r->type == XCB_ATOM_NONE)
			  return;
		data = xcb_get_property_value(r);
		const int l = xcb_get_property_value_length(r);
		nread += l;
		bytes_after = r->bytes_after;
		PASTE(data, l);
		free(r);
	} while (bytes_after > 0);
}

