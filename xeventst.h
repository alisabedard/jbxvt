#ifndef XEVENTST_H
#define XEVENTST_H

#include <stdint.h>

/*  Small X event structure used to queue interesting X events that need to
 *  be converted into tokens.
 */
struct xeventst {
	struct xeventst *xe_next;
	struct xeventst *xe_prev;
	uint32_t xe_time;
	uint32_t xe_requestor; // selections
	uint32_t xe_window;
	uint32_t xe_property; // selections
	uint32_t xe_target;
	uint32_t xe_detail;
	uint32_t xe_type;
	uint32_t xe_button;
	uint32_t xe_state;
	int16_t xe_x;
	int16_t xe_y;
	uint16_t xe_width;
	uint16_t xe_height;
};

#endif//!XEVENTST_H
