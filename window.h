/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_WINDOW_H
#define JBXVT_WINDOW_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
xcb_window_t jbxvt_get_main_window(xcb_connection_t * xc);
xcb_window_t jbxvt_get_vt_window(xcb_connection_t * xc);
// Set main window property string
void jbxvt_set_property(xcb_connection_t * xc,
	const xcb_atom_t property, const uint32_t data_len,
	uint8_t * data);
// Change window or icon name:
void jbxvt_change_name(xcb_connection_t * xc,
	uint8_t * restrict str, const bool icon);
void jbxvt_map_window(xcb_connection_t * xc);
void jbxvt_resize_window(xcb_connection_t * xc);
#endif//JBXVT_WINDOW_H
