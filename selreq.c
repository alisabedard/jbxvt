/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "selreq.h"
#include <stdlib.h>
#include <unistd.h>
#include "command.h"
#include "config.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "selection.h"
#include "window.h"
// Returns the length pasted
static size_t paste(const uint8_t * data, const size_t length)
{
    if (data && length)
        jb_check(write(jbxvt_get_fd(), data, length) != -1,
            "Cannot paste");
    return length;
}
static bool reply_is_invalid(xcb_get_property_reply_t * r)
{
    if (!r) // no reply received, invalid
        return true;
    if (r->type == XCB_ATOM_NONE) {
        free(r);
        return true; // no data in reply, invalid
    }
    return false; // reply is valid
}
static xcb_get_property_cookie_t get_prop(xcb_connection_t * xc,
    const xcb_atom_t clipboard, const uint32_t already_read)
{
    // divide by 4 to convert 32 bit words to bytes
    return xcb_get_property(xc, false, jbxvt_get_main_window(xc),
        clipboard, XCB_ATOM_ANY, already_read / 4, JBXVT_PROP_SIZE);
}
static void request_conversion(xcb_connection_t * xc,
    const xcb_window_t win, const xcb_atom_t cb, const xcb_timestamp_t t)
{
    xcb_convert_selection(xc, win, cb,
        XCB_ATOM_STRING, cb, t);
    xcb_flush(xc);
    // This prevents pasting stale data:
    free(xcb_wait_for_event(xc)); // discard
}
static bool paste_from(xcb_connection_t * xc,
    const xcb_atom_t cb, const xcb_timestamp_t t)
{
    LOG("paste_from(clipboard: %d, timestamp: %d)", cb, t);
    request_conversion(xc, jbxvt_get_main_window(xc), cb, t);
    xcb_get_property_reply_t * r = xcb_get_property_reply(xc,
        get_prop(xc, cb, 0), NULL);
    if (reply_is_invalid(r))
        return false;
    paste(xcb_get_property_value(r),
        (size_t)xcb_get_property_value_length(r));
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
static uint32_t paste_primary_chunk(xcb_connection_t * xc,
    const xcb_atom_t property, uint32_t nread)
{
    xcb_get_property_reply_t * r = xcb_get_property_reply(xc,
        get_prop(xc, property, nread), NULL);
    if (!reply_is_invalid(r)) {
        const uint32_t bytes_after = r->bytes_after;
        nread += paste(xcb_get_property_value(r),
            (size_t)xcb_get_property_value_length(r));
        free(r);
        if (bytes_after > 0)
            return paste_primary_chunk(xc, property, nread);
    }
    return nread;
}
//  Respond to a notification that a primary selection has been sent
uint32_t jbxvt_paste_primary(xcb_connection_t * xc,
    const xcb_timestamp_t t, const xcb_window_t window,
    const xcb_atom_t property)
{
    LOG("paste_primary(time: %d, win: %d, prop: %d)",
        (int)t, (int)window, (int)property);
    request_conversion(xc, window, property, t);
    return paste_primary_chunk(xc, property, 0);
}
