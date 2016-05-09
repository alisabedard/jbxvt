#ifndef SCR_REQ_SEL_H
#define SCR_REQ_SEL_H

#include <X11/Xatom.h>
#include <X11/Xlib.h>

//  Request the current primary selection
void scr_request_selection(int time, int x, int y);

//  Respond to a notification that a primary selection has been sent
void scr_paste_primary(const int time __attribute__((unused)),
	const Window window, const Atom property);


#endif//!SCR_REQ_SEL_H
