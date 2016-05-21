/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef XSETUP_H
#define XSETUP_H

#include <stdbool.h>
#include <stdint.h>
#include <X11/Xutil.h>

extern XSizeHints sizehints;

void switch_scrollbar(void);

// Change window and/or icon name:
void change_name(uint8_t * restrict str, const bool window,
	const bool icon);

int resize_window(void);
void send_auth(void);
void map_window(void);
bool is_logshell(void);
void usage(int);

#endif//XSETUP_H
