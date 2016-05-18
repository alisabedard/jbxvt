/*  Copyright 1992 John Bovey, University of Kent at Canterbury.
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

/* @(#)ttyinit.h	1.1 16/11/93 (UKC) */

#ifndef TTYINIT_H
#define TTYINIT_H

#include <stdint.h>

#ifdef LINUX
#include <asm-generic/ioctls.h>
#endif//LINUX

void quit(int) __attribute__((noreturn));
#ifdef TIOCSWINSZ
void tty_set_size(const uint8_t width, const uint8_t height);
#else//!TIOCSWINSZ
#define tty_set_size(w, h)
#endif//TIOCSWINSZ
int run_command(char ** restrict argv);

#endif//!TTYINIT_H

