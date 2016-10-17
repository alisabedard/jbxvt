// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVTEVENT_H
#define JBXVTEVENT_H
#include <stdint.h>
#include <xcb/xproto.h>
/*  Small X event structure used to queue interesting X events
    that need to be converted into tokens.  */
struct JBXVTEvent {
	xcb_timestamp_t time;
	xcb_window_t window;
	xcb_atom_t property; // selections
	union {
		xcb_atom_t target;
		uint16_t state;
	};
	union {
		xcb_window_t requestor; // selections
		struct xcb_rectangle_t box;
	};
	union {
		uint8_t detail;
		uint8_t button;
	};
	uint8_t type;
};
#endif//!JBXVTEVENT_H
