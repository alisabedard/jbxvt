#ifndef WM_DEL_WIN_H
#define WM_DEL_WIN_H

#include <X11/Xlib.h>

void init_wm_del_win(void);
Atom get_wm_del_win(void) __attribute__((pure));

#endif//!WM_DEL_WIN_H
