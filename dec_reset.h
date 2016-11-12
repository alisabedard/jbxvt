/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_DEC_RESET_H
#define JBXVT_DEC_RESET_H
#include "JBXVTToken.h"
#include <xcb/xcb.h>
void jbxvt_dec_reset(xcb_connection_t * xc, struct JBXVTToken * restrict token)
	__attribute__((nonnull));
#endif//!JBXVT_DEC_RESET_H
