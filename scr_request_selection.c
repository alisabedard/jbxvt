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
	LOG("send_selection(%s, count: %d)", str, count);
	if (!count)
		  return;
	for (char * i = (char *)str; (i = strchr(i, '\n')); *i = '\r')
		  ;
	jbxvt.com.send_nxt = str;
	jbxvt.com.send_count = count;
}

static void paste_root(void)
{
	LOG("using root window XCB_ATOM_CUT_BUFFER0");
	xcb_get_property_cookie_t c = xcb_get_property(jbxvt.X.xcb, false,
		jbxvt.X.screen->root, XCB_ATOM_CUT_BUFFER0,
		XCB_ATOM_STRING, 0, PROP_SIZE);
	xcb_get_property_reply_t * r = xcb_get_property_reply(
		jbxvt.X.xcb, c, NULL);
	send_selection(xcb_get_property_value(r),
		xcb_get_property_value_length(r));
	free(r);
}

static bool paste_from(const xcb_atom_t clipboard, const xcb_timestamp_t t)
{
	xcb_get_selection_owner_cookie_t o_c;
	xcb_get_property_cookie_t p_c;
	xcb_get_selection_owner_reply_t * o_r;
	xcb_get_property_reply_t * p_r;
	xcb_connection_t * x = jbxvt.X.xcb;
	xcb_generic_error_t *e;

	o_c = xcb_get_selection_owner_unchecked(x, clipboard);
	o_r = xcb_get_selection_owner_reply(x, o_c, &e);
	if (e) {
		free(o_r);
		return false;
	}
	xcb_convert_selection(x, o_r->owner, clipboard,
		XCB_ATOM_STRING, clipboard, t);
#if 0
	xcb_convert_selection(x, o_r->owner, clipboard,
		XCB_ATOM_STRING, clipboard, XCB_CURRENT_TIME);
#endif
	p_c = xcb_get_property(x, false, o_r->owner, clipboard, XCB_ATOM_ANY,
		0, PROP_SIZE);
	free(o_r);
	p_r = xcb_get_property_reply(x, p_c, NULL);
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
	/* FIXME:  For some reason, the CLIPBOARD selection retrieved
	   first matches the old selection, then updates with a second
	   paste.  Why?  */
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
			PROP_SIZE);
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

