// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVTEVENT_H
#define JBXVTEVENT_H

#include <stdint.h>
#include <xcb/xproto.h>

/*  Small X event structure used to queue interesting X events that need to
 *  be converted into tokens.  */
struct JBXVTEvent {
	struct JBXVTEvent *next;
	struct JBXVTEvent *prev;
	uint8_t type;
	union {
		uint8_t detail;
		uint8_t button;
	};
	xcb_timestamp_t time;
	xcb_window_t window;
	xcb_atom_t property; // selections
	union {
		xcb_atom_t target;
		uint16_t state;
	};
	union {
		struct xcb_rectangle_t box;
		xcb_window_t requestor; // selections
	};
};

#endif//!JBXVTEVENT_H
