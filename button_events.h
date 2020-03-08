/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_BUTTON_EVENTS_H
#define JBXVT_BUTTON_EVENTS_H
#include <xcb/xcb.h>
void jbxvt_handle_button_press_event(xcb_connection_t * xc,
    xcb_button_press_event_t * e);
void jbxvt_handle_button_release_event(xcb_connection_t * xc,
    xcb_button_release_event_t * e);
#endif//!JBXVT_BUTTON_EVENTS_H
