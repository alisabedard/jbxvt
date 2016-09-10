/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef COMMAND_H
#define COMMAND_H

#include "libjb/size.h"
#include <stdint.h>

enum CommandLimits {
	KBUFSIZE =	8,		// size of keyboard mapping buffer
	COM_BUF_SIZE =	UINT8_MAX,	// size of command read buffer
	COM_PUSH_MAX =	20,		// max # chars to put back to input queue
	MP_INTERVAL =	500		// multi-press interval in ms
};

//JBXVTEvent * pop_xevent(void);

#ifndef __clang__
char * cprintf(char *, ...)
	__attribute__((format(gnu_printf, 1, 2)));
#else//__clang__
char * cprintf(char *, ...);
#endif//!__clang__

/*  Initialise the command connection.  This should be called after the X
 *  server connection is established.  */
void init_command(char ** restrict argv);

//  Push an input character back into the input queue.
void put_com_char(const uint8_t c);

#ifdef LINUX
#include <asm-generic/ioctls.h>
#endif//LINUX

#ifdef TIOCSWINSZ
void tty_set_size(const struct JBDim sz);
#else//!TIOCSWINSZ
#define tty_set_size(dim)
#endif//TIOCSWINSZ

#endif//!COMMAND_H
