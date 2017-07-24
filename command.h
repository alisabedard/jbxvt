/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_COMMAND_H
#define JBXVT_COMMAND_H
#include <stdint.h>
#include "libjb/util.h"
struct JBDim;
fd_t jbxvt_get_fd(void);
/*  Initialise the command connection.  This should be called after the X
 *  server connection is established.  */
void jbxvt_init_command_module(char ** restrict argv);
#ifdef LINUX
#ifdef HAVE_ASM_GENERIC_IOCTLS_H
#include <asm-generic/ioctls.h>
#endif//HAVE_ASM_GENERIC_IOCTLS_H
#endif//LINUX
#ifdef TIOCSWINSZ
void jbxvt_set_tty_size(const struct JBDim sz);
#else//!TIOCSWINSZ
#define jbxvt_set_tty_size(dim)
#endif//TIOCSWINSZ
#endif//!JBXVT_COMMAND_H
