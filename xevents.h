/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef JBXVT_XEVENTS_H
#define JBXVT_XEVENTS_H

#include "tokenst.h"

// convert next X event into a token
bool handle_xevents(Token * restrict tk);

#endif//!JBXVT_XEVENTS_H
