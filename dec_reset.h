/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef DEC_RESET_H
#define DEC_RESET_H

#include "tokenst.h"

void dec_reset(Token * restrict token)
	__attribute__((nonnull));

#endif//!DEC_RESET_H