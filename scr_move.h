#ifndef SCR_MOVE_H
#define SCR_MOVE_H
#include <stdint.h>
enum ScrMoveFlags {
	COL_RELATIVE = 1, // column movement is relative
	ROW_RELATIVE = 2  // row movement is relative
};
/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void jbxvt_move(const int16_t x, const int16_t y, const uint8_t relative);
// Ensure cursor coordinates are valid per screen and decom mode
// Returns new cursor y value
int16_t reset_row_col(void);
#endif//!SCR_MOVE_H
