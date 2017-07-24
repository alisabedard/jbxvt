/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_LOOKUP_KEY_H
#define JBXVT_LOOKUP_KEY_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
// Set key mode for cursor keys if is_cursor, else for keypad keys
void jbxvt_set_keys(const bool mode_high, const bool is_cursor);
// Convert the keypress event into a string
uint8_t *jbxvt_lookup_key(xcb_connection_t * restrict xc,
	void * restrict ev, int_fast16_t * restrict pcount)
	__attribute__((nonnull));
#endif//JBXVT_LOOKUP_KEY_H
