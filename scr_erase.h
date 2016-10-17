/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SCR_ERASE_H
#define JBXVT_SCR_ERASE_H
#include <stdint.h>
enum {
	JBXVT_ERASE_AFTER = 0,
	JBXVT_ERASE_BEFORE = 1,
	JBXVT_ERASE_ALL = 2,
	JBXVT_ERASE_SAVED = 3
};
//  erase part or the whole of a line
void jbxvt_erase_line(const int8_t mode);
//  erase part or the whole of the screen
void jbxvt_erase_screen(const int8_t mode);
#endif//!JBXVT_SCR_ERASE_H
