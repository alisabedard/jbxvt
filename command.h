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

/* @(#)command.h	1.2 16/11/93 (UKC) */

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
void get_token(struct tokenst *);
void init_command(char *,char **);
unsigned char *lookup_key(XEvent *,int *);
void push_com_char(int);
void push_xevent(struct xeventst *);
void send_string(unsigned char *,int);
void set_cur_keys(int);
void set_kp_keys(int);

#ifdef TK_DEBUG
void show_token(struct tokenst *);
#endif//TK_DEBUG

#endif//!COMMAND_H
