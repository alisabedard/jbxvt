#ifndef XEVENTST_H
#define XEVENTST_H

#include <stdint.h>

/*  Small X event structure used to queue interesting X events that need to
 *  be converted into tokens.  */
typedef struct xeventst {
	struct xeventst *next;
	struct xeventst *prev;
	uint8_t type;
	union {
		uint8_t detail;
		uint8_t button;
	};
	xcb_timestamp_t time;
	xcb_window_t window;
	xcb_window_t requestor; // selections
	xcb_atom_t property; // selections
	xcb_atom_t target;
	xcb_rectangle_t box;
	uint16_t state;
} JBXVTEvent;

#endif//!XEVENTST_H
