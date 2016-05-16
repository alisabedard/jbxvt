/*  Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.
 *
 *  Redistribution and use in source code and/or executable forms, with
 *  or without modification, are permitted provided that the following
 *  condition is met:
 *
 *  Any redistribution must retain the above copyright notice, this
 *  condition and the following disclaimer, either as part of the
 *  program source code included in the redistribution or in human-
 *  readable materials provided with the redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS".  Any express or implied
 *  warranties concerning this software are disclaimed by the copyright
 *  holder to the fullest extent permitted by applicable law.  In no
 *  event shall the copyright-holder be liable for any damages of any
 *  kind, however caused and on any theory of liability, arising in any
 *  way out of the use of, or inability to use, this software.
 *
 *  -------------------------------------------------------------------
 *
 *  In other words, do not misrepresent my work as your own work, and
 *  do not sue me if it causes problems.  Feel free to do anything else
 *  you wish with it.
 */
/* @(#)xvt.h	1.3 16/11/93 (UKC) */
#ifndef XVT_H
#define XVT_H

#define VERSION "2.2"	/* Overall release number of the current version */

enum { MARGIN = 2 };	/* gap between the text and the window edges */

/*  Some wired in defaults so we can run without any external resources.
 */
#define DEF_FONT "lucidasanstypewriter-10"
#define FIXED_FONT "fixed"	/* last resort font */
#define DEF_SAVED_LINES 512 /* number of saved lines that have
			       scrolled off the top */
#define TERM_ENV "rxvt-16color"
#define COMMAND "/bin/sh"	/* default command to run */

/* arguments to set and reset functions.
 */
enum ModeValue { LOW, HIGH };

#endif//!XVT_H
