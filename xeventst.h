#ifndef XEVENTST_H
#define XEVENTST_H

#include <stdint.h>
#include <X11/Xlib.h>

/*  Small X event structure used to queue interesting X events that need to
 *  be converted into tokens.
 */
struct xeventst {
	struct xeventst *xe_next;
	struct xeventst *xe_prev;
	Time xe_time;
	Window xe_requestor; // selections
	Window xe_window;
	Atom xe_property; // selections
	Atom xe_target;
	int32_t xe_detail;
	int32_t xe_type;
	int32_t xe_button;
	int32_t xe_state;
	int16_t xe_x;
	int16_t xe_y;
	uint16_t xe_width;
	uint16_t xe_height;
};

#endif//!XEVENTST_H
