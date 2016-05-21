/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef COMMAND_H
#define COMMAND_H

#include "tokenst.h"
#include "xeventst.h"
#include <X11/Xlib.h>

enum CommandLimits {
	NLMAX =		15,	// max number of lines to scroll
	KBUFSIZE =	256,	// size of keyboard mapping buffer
	COM_BUF_SIZE =	512,	// size of command read buffer
	COM_PUSH_MAX =	20,	// max # chars to push back to input queue
	MP_INTERVAL = 500	// multi-press interval in ms
};

//  Special character returned by get_com_char().
#define GCC_NULL	0x100		/* Input buffer is empty */
#define ESC		033

//  Flags used to control get_com_char();
enum ComCharFlags {BUF_ONLY=1, GET_XEVENTS=2};

struct xeventst *pop_xevent(void);
void cprintf(char *,...);
void init_command(char ** restrict argv);
uint8_t *lookup_key(XEvent *,int *);
void push_com_char(int);
void push_xevent(struct xeventst *);
void send_string(uint8_t *,int);
void set_cur_keys(int);
void set_kp_keys(int);

#ifdef TK_DEBUG
void show_token(struct tokenst *);
#endif//TK_DEBUG

#endif//!COMMAND_H
