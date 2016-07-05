#ifndef XEVENTST_H
#define XEVENTST_H

#include <stdint.h>

/*  Small X event structure used to queue interesting X events that need to
 *  be converted into tokens.  */
typedef struct xeventst {
	struct xeventst *next;
	struct xeventst *prev;
	uint32_t time;
	uint32_t requestor; // selections
	uint32_t window;
	uint32_t property; // selections
	uint32_t target;
	uint32_t detail;
	uint32_t type;
	uint32_t button;
	uint32_t state;
	xcb_rectangle_t box;
} JBXVTEvent;

#endif//!XEVENTST_H
