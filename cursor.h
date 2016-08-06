/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_CURSOR_H
#define JBXVT_CURSOR_H

void save_cursor(void);

void restore_cursor(void);

//  Draw the cursor at the current position.
void draw_cursor(void);

#endif//!JBXVT_CURSOR_H
