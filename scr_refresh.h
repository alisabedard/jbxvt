/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCR_REFRESH_H
#define SCR_REFRESH_H

#include "jbxvt.h"

/*  Refresh the region of the screen delimited by the aruments.  Used to
 *  repair after minor exposure events.  */
void scr_refresh(const Dim pos, const Dim sz);

#endif//SCR_REFRESH_H
