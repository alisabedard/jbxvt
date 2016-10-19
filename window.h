/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_WINDOW_H
#define JBXVT_WINDOW_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>
// Set main window property string
void jbxvt_set_property(const xcb_atom_t prop, const size_t sz,
	uint8_t * value);
// Change window or icon name:
void jbxvt_change_name(uint8_t * restrict str, const bool icon);
void jbxvt_map_window(void);
void jbxvt_resize_window(void);
#endif//JBXVT_WINDOW_H
