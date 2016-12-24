#ifndef JBXVT_SCR_MOVE_H
#define JBXVT_SCR_MOVE_H
#include <stdint.h>
#include <xcb/xcb.h>
enum JBXVTMoveFlags {
	JBXVT_COLUMN_RELATIVE = 1, // column movement is relative
	JBXVT_ROW_RELATIVE = 2  // row movement is relative
};
/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void jbxvt_move(xcb_connection_t * xc,
	const int16_t x, const int16_t y, const uint8_t relative);
// Ensure cursor coordinates are valid per screen and decom mode
// Returns new cursor y value
int16_t jbxvt_check_cursor_position(void);
#endif//!JBXVT_SCR_MOVE_H
