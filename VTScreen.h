/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREENST_H
#define SCREENST_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>

typedef enum {
	CHARSET_GB, CHARSET_ASCII, CHARSET_SG0, CHARSET_SG1, CHARSET_SG2
} CharacterSet;

/*  Structure describing the current state of the screen.
 */
typedef struct {
	uint8_t **text;		// backup copy of text
	uint32_t **rend;	// rendition styles
	Size margin;		// scroll margins, top and bottom
	xcb_point_t cursor;	// cursor position, row and column
	CharacterSet charset[2];// graphics mode char set
	bool decom:1;		// origin mode flag
	bool decawm:1;		// DECAWM auto-wrap flag
	bool wrap_next:1;	// wrap before the next printed character
	bool insert:1;		// insert mode flag
	bool decanm:1;		// DECANM -- ANSI/VT52
	bool dectcem:1;		// DECTCEM -- hide cursor
	bool att610:1;		// stop blinking cursor
	bool ptr_xy:1;		// send x y on button press/release
	bool ptr_cell:1;	// cell motion mouse tracking
	bool decpm:1;		// privacy message
	bool mouse_x10:1;	// ptr coord on button press
	bool mouse_vt200:1;	// ptr press+release
	bool mouse_vt200hl:1;	// highlight tracking
	bool mouse_btn_evt:1;	// button event tracking
	bool mouse_any_evt:1;	// all motion tracking
	bool mouse_focus_evt:1; // focus tracking
	bool mouse_ext:1;	// UTF-8 coords
	bool mouse_sgr:1;	// sgr scheme
	bool mouse_urxvt:1;	// decimal integer coords
	bool mouse_alt_scroll:1;// send cursor up/down instead
	uint8_t charsel:1;	// charset index
} VTScreen;

#endif//!SCREENST_H
