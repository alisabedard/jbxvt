#ifndef SCR_MOVE_H
#define SCR_MOVE_H

#include <stdint.h>

/*  Move the cursor to a new position.  The relative argument is a pair of
 *  flags that specify relative rather than absolute motion.  */
void scr_move(const int16_t x, const int16_t y, const uint8_t relative);

#endif//!SCR_MOVE_H
