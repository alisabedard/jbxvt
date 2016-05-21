/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_CONFIG_H
#define JBXVT_CONFIG_H

#define VERSION "0.2" // Overall release number of the current version

#define DEF_FONT "lucidasanstypewriter-10"
#define FIXED_FONT "fixed" // last resort font
#define TERM_ENV "xterm" // TERM env var to set
#define COMMAND "/bin/sh" // default command to run

enum { 	SBAR_WIDTH = 12, // width of scroll bar
	MARGIN = 2, // gap between the text and the window edges
	DEF_SAVED_LINES = 75 // # lines of scroll history
};

#endif//!JBXVT_CONFIG_H
