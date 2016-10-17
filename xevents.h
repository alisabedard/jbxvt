/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_XEVENTS_H
#define JBXVT_XEVENTS_H
#include "JBXVTEvent.h"
#include <stdbool.h>
xcb_atom_t wm_del_win(void);
// Handle X11 event described by xe
bool handle_xevents(struct JBXVTEvent * xe);
#endif//!JBXVT_XEVENTS_H
