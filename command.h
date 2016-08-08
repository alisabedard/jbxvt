/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

enum CommandLimits {
	KBUFSIZE =	8,	// size of keyboard mapping buffer
	COM_BUF_SIZE =	512,	// size of command read buffer
	COM_PUSH_MAX =	20,	// max # chars to push back to input queue
	MP_INTERVAL =	500	// multi-press interval in ms
};

//JBXVTEvent * pop_xevent(void);

char * cprintf(char *, ...);

/*  Initialise the command connection.  This should be called after the X
 *  server connection is established.  */
void init_command(char ** restrict argv);

//  Push an input character back into the input queue.
void push_com_char(const uint8_t c);

#ifdef LINUX
#include <asm-generic/ioctls.h>
#endif//LINUX

#ifdef TIOCSWINSZ
void tty_set_size(const uint8_t width, const uint8_t height);
#else//!TIOCSWINSZ
#define tty_set_size(w, h)
#endif//TIOCSWINSZ

#endif//!COMMAND_H
