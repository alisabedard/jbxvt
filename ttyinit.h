/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef TTYINIT_H
#define TTYINIT_H

#include <stdint.h>

#define QUIT_DISPLAY	"Bad DISPLAY"
#define QUIT_ERROR	"Fatal error"
#define QUIT_SESSION	"Cannot start session"
#define QUIT_SIGNAL	"Got signal"
#define QUIT_TTY	"Cannot open tty"
#define WARN_UTMP	"Cannot open UTMP file"

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
int run_command(char ** restrict argv);

#endif//!TTYINIT_H

