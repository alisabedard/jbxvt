#ifndef XEVENTST_H
#define XEVENTST_H

#include <X11/Xlib.h>

/*  Small X event structure used to queue interesting X events that need to
 *  be converted into tokens.
 */
struct xeventst {
	int xe_type;
	int xe_button;
	int xe_state;
	int xe_x;
	int xe_y;
	int xe_width;
	int xe_height;
	int xe_detail;
	unsigned long xe_time;
	Window xe_window;
	Atom xe_property;		/* for selection requests */
	Atom xe_target;
	Window xe_requestor;		/* ditto */
	struct xeventst *xe_next;
	struct xeventst *xe_prev;
};

#endif//!XEVENTST_H
