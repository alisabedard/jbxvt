/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef COMMAND_H
#define COMMAND_H

#include "tokenst.h"
#include "xeventst.h"
#include "xvt.h"

#include <stdbool.h>
#include <X11/Xlib.h>

enum CommandLimits {
	NLMAX =		15,	// max number of lines to scroll
	KBUFSIZE =	256,	// size of keyboard mapping buffer
	COM_BUF_SIZE =	512,	// size of command read buffer
	COM_PUSH_MAX =	20,	// max # chars to push back to input queue
	MP_INTERVAL = 	500	// multi-press interval in ms
};

//  Special character returned by get_com_char().
enum ComCharReturn {
	GCC_NULL = 0x100, // Input buffer is empty
	ESC = 033
};

//  Flags used to control get_com_char();
enum ComCharFlags {BUF_ONLY=1, GET_XEVENTS=2};

struct xeventst *pop_xevent(void);
void cprintf(char *,...);

/*  Initialise the command connection.  This should be called after the X
 *  server connection is established.  */
void init_command(char ** restrict argv);

// Convert the keypress event into a string
uint8_t *lookup_key(XEvent * restrict ev, int16_t * restrict pcount);

void push_com_char(int);
void push_xevent(struct xeventst *);

//  Send count characters directly to the command.
void send_string(uint8_t * restrict buf, const uint8_t count);

// Set key mode for cursor keys if is_cursor, else for keypad keys
void set_keys(const enum ModeValue mode, const bool is_cursor);
#define set_cur_keys(mode) set_keys(mode, true)
#define set_kp_keys(mode) set_keys(mode, false)

#ifdef TK_DEBUG
void show_token(struct tokenst *);
#endif//TK_DEBUG

#endif//!COMMAND_H
