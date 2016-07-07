/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

enum CommandLimits {
	NLMAX =		15,	// max number of lines to scroll
	KBUFSIZE =	8,	// size of keyboard mapping buffer
	COM_BUF_SIZE =	512,	// size of command read buffer
	COM_PUSH_MAX =	20,	// max # chars to push back to input queue
	MP_INTERVAL =	500	// multi-press interval in ms
};

struct xeventst *pop_xevent(void);

void cprintf(char *, ...);

/*  Initialise the command connection.  This should be called after the X
 *  server connection is established.  */
void init_command(char ** restrict argv);

//  Push an input character back into the input queue.
void push_com_char(const uint8_t c);

//  Push a mini X event onto the queue
void push_xevent(struct xeventst *);

//  Send count characters directly to the command.
void send_string(uint8_t * restrict buf, const uint8_t count);

#endif//!COMMAND_H
