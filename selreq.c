/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "selreq.h"

#include "config.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/util.h"

#include <alloca.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//  Send the selection to the command
static void send_selection(uint8_t * str, const uint16_t count)
{
	jb_check(write(jbxvt.com.fd, str, count) != -1,
		"Could not write to command");
}

static void paste_root(void)
{
	LOG("using root window XCB_ATOM_CUT_BUFFER0");
	xcb_get_property_cookie_t c = xcb_get_property(jbxvt.X.xcb, false,
		jbxvt.X.screen->root, XCB_ATOM_CUT_BUFFER0,
		XCB_ATOM_STRING, 0, JBXVT_PROP_SIZE);
	xcb_get_property_reply_t * r = xcb_get_property_reply(
		jbxvt.X.xcb, c, NULL);
	send_selection(xcb_get_property_value(r),
		xcb_get_property_value_length(r));
	free(r);
}

static bool paste_from(const xcb_atom_t clipboard, const xcb_timestamp_t t)
{
	struct JBXVTXData * X = &jbxvt.X;
	xcb_connection_t * x = X->xcb;
	xcb_delete_property(x, jbxvt.X.win.vt, XCB_ATOM_STRING);
	xcb_void_cookie_t c = xcb_convert_selection_checked(x,
		jbxvt.X.win.vt, clipboard,
		XCB_ATOM_STRING, XCB_ATOM_STRING, t);
	xcb_generic_error_t * e = xcb_request_check(x, c);
	if (jb_check(!e, "Could not convert selection")) {
		free(e);
		return false;
	}
	// Wait for the conversion to occur:
	free(xcb_wait_for_event(x)); // Discard event
	xcb_get_property_cookie_t p_c = xcb_get_property(x, true,
		jbxvt.X.win.vt, XCB_ATOM_STRING, XCB_ATOM_ANY, 0,
		JBXVT_PROP_SIZE);
	xcb_get_property_reply_t * p_r = xcb_get_property_reply(x, p_c, NULL);
	if (!p_r)
		  return false;
	if (p_r->type == XCB_ATOM_NONE) {
		free(p_r);
		return false;
	}
	send_selection(xcb_get_property_value(p_r),
		xcb_get_property_value_length(p_r));
	free(p_r);
	return true;
}

void request_selection(const xcb_timestamp_t t)
{
	if (paste_from(XCB_ATOM_PRIMARY, t))
		  return;
	if (paste_from(jbxvt.X.clipboard, t))
		  return;
	if (paste_from(XCB_ATOM_SECONDARY, t))
		  return;
	paste_root();
}

//  Respond to a notification that a primary selection has been sent
void paste_primary(const xcb_window_t window, const xcb_atom_t property)
{
	LOG("paste_primary()");
	if (property == XCB_NONE)
		return;
	uint32_t nread = 0, bytes_after;
	xcb_connection_t * restrict x = jbxvt.X.xcb;
	xcb_convert_selection(x, window, property,
		XCB_ATOM_STRING, property, XCB_CURRENT_TIME);
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
		send_selection(data, l);
		free(r);
	} while (bytes_after > 0);
}

