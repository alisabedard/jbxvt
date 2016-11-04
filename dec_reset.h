/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_DEC_RESET_H
#define JBXVT_DEC_RESET_H
#include "Token.h"
#include <xcb/xcb.h>
void jbxvt_dec_reset(xcb_connection_t * xc, struct Token * restrict token)
	__attribute__((nonnull));
// Restore private modes.
void jbxvt_restore_mode(void);
// Save private modes.
void jbxvt_save_mode(void);
#endif//!JBXVT_DEC_RESET_H
