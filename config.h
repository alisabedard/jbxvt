/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_CONFIG_H
#define JBXVT_CONFIG_H

#define JBXVT_VERSION "0.5" // Overall release number of the current version

#define FALLBACK_FONT "fixed"
#define JBXVT_FONT "-*-terminus-medium-r-*-*-14-*-*-*-*-*-iso10646-*"
#define JBXVT_BOLD_FONT "-*-terminus-bold-r-*-*-14-*-*-*-*-*-iso10646-*"

#define TERM_ENV "xterm-256color" // TERM env var to set
#define COMMAND "/bin/sh" // default command to run

#define JBXVT_FG "white"
#define JBXVT_BG "black"

enum {	JBXVT_SCROLLBAR_WIDTH = 10, // width of scroll bar
	JBXVT_MARGIN = 1, // gap between the text and the window edges
	JBXVT_ROWS = 24, // default # rows
	JBXVT_COLUMNS = 80, // default # columns

	JBXVT_MAX_SCROLL = 100,

	// Set these to fit your display :
	JBXVT_MAX_COLS = 170, // max columns on screen
	JBXVT_MAX_ROWS = 60, // max rows on screen

	JBXVT_PROP_SIZE = 1024, // selection property chunk size
	JBXVT_SEL_KEY_DEL = 2000, // ms for kb input before selection
};

#endif//!JBXVT_CONFIG_H
