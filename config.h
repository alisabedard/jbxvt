/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_CONFIG_H
#define JBXVT_CONFIG_H

#define VERSION "0.3" // Overall release number of the current version

#define FALLBACK_FONT "fixed"
#define DEF_FONT "-*-terminus-medium-r-*-*-14-*-*-*-*-*-iso10646-*"
#define BOLD_FONT "-*-terminus-bold-r-*-*-14-*-*-*-*-*-iso10646-*"

#define TERM_ENV "xterm-256color" // TERM env var to set
#define COMMAND "/bin/sh" // default command to run

enum {	SBAR_WIDTH = 10, // width of scroll bar
	MARGIN = 1, // gap between the text and the window edges
	JBXVT_ROWS = 24, // default # rows
	JBXVT_COLUMNS = 80 // default # columns
};

#endif//!JBXVT_CONFIG_H
