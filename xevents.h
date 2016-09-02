/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_XEVENTS_H
#define JBXVT_XEVENTS_H

#include "Token.h"
#include <stdbool.h>
#include <xcb/xcb.h>

xcb_atom_t wm_del_win(void);

// convert next X event into a token
bool handle_xevents(struct Token * restrict tk);

#endif//!JBXVT_XEVENTS_H
