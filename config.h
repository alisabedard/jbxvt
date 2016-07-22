/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_CONFIG_H
#define JBXVT_CONFIG_H

#define VERSION "0.4" // Overall release number of the current version

#define FALLBACK_FONT "fixed"
#define DEF_FONT "-*-terminus-medium-r-*-*-14-*-*-*-*-*-iso10646-*"
#define BOLD_FONT "-*-terminus-bold-r-*-*-14-*-*-*-*-*-iso10646-*"

#define TERM_ENV "xterm-256color" // TERM env var to set
#define COMMAND "/bin/sh" // default command to run

#define JBXVT_FG "white"
#define JBXVT_BG "black"

enum {	SBAR_WIDTH = 10, // width of scroll bar
	MARGIN = 1, // gap between the text and the window edges
	JBXVT_ROWS = 24, // default # rows
	JBXVT_COLUMNS = 80, // default # columns
	MAX_SCROLL = 126, /* max # lines that can scroll at once.
			    126 is greatest value supported. */
	JBXVT_MAX_COLS = 255,
	JBXVT_MAX_ROWS = 255,
	JBXVT_PROP_SIZE = 1024, // selection property chunk size
	JBXVT_SEL_KEY_DEL = 2000, // ms for kb input before selection
};

#endif//!JBXVT_CONFIG_H
