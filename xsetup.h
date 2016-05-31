/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef XSETUP_H
#define XSETUP_H

#include <stdbool.h>
#include <stdint.h>


// Change window and/or icon name:
void change_name(uint8_t * restrict str, const bool window,
	const bool icon);

void map_window(void);
void resize_window(void);
void send_auth(void);
void switch_scrollbar(void);
void usage(int);

#endif//XSETUP_H
