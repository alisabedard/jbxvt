/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_XEVENTS_H
#define JBXVT_XEVENTS_H
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
xcb_atom_t jbxvt_get_wm_del_win(xcb_connection_t * xc);
// Handle X event on queue.  Return true if event was handled.
bool jbxvt_handle_xevents(xcb_connection_t * xc);
#endif//!JBXVT_XEVENTS_H
