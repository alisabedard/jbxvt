/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef TTYINIT_H
#define TTYINIT_H

#include "jbxvt.h"

#ifdef LINUX
#include <asm-generic/ioctls.h>
#endif//LINUX

void quit(const int8_t status, const char * restrict msg)
	__attribute__((noreturn));
#ifdef TIOCSWINSZ
void tty_set_size(const uint8_t width, const uint8_t height);
#else//!TIOCSWINSZ
#define tty_set_size(w, h)
#endif//TIOCSWINSZ
fd_t run_command(char ** restrict argv);

#endif//!TTYINIT_H

