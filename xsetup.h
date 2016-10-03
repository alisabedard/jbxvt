/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_XSETUP_H
#define JBXVT_XSETUP_H

#include <stdbool.h>
#include <stdint.h>

// Change window or icon name:
void change_name(uint8_t * restrict str, const bool icon);

void map_window(void);

void resize_window(void);

#endif//JBXVT_XSETUP_H
