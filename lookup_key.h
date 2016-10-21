/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_LOOKUP_KEY_H
#define JBXVT_LOOKUP_KEY_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
/*  Thanks to Rob McMullen for the following function key mapping tables
 *  and code.  */
/*  Structure used to describe the string generated by a function key,
 *  keypad key, etc.  */
struct JBXVTKeyStrings {
	uint8_t ks_type;	// the way to generate the string (see below)
	uint8_t ks_value;	// value used in creating the string
};
/*  Different values for ks_type which determine how the value is used to
 *  generate the string.  */
enum KeySymType {
	KS_TYPE_NONE,		// No output
	KS_TYPE_CHAR,           // as printf("%c",ks_value)
	KS_TYPE_XTERM,          // as printf("\033[%d",ks_value)
	KS_TYPE_APPKEY,         // as printf("\033O%c",ks_value)
	KS_TYPE_NONAPP          // as printf("\033[%c",ks_value)
};
//  Structure used to map a keysym to a string.
struct JBXVTKeyMaps {
	xcb_keysym_t km_keysym;
	struct JBXVTKeyStrings km_normal;	/* The usual string */
	struct JBXVTKeyStrings km_alt;	/* The alternative string */
};
// Set key mode for cursor keys if is_cursor, else for keypad keys
void jbxvt_set_keys(const bool mode_high, const bool is_cursor);
// Convert the keypress event into a string
uint8_t *jbxvt_lookup_key(void * restrict ev, int_fast16_t * restrict pcount)
	__attribute__((nonnull));
#endif//JBXVT_LOOKUP_KEY_H
