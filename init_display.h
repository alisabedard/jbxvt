#ifndef INIT_DISPLAY_H
#define INIT_DISPLAY_H

#include <stdbool.h>
#include <X11/Xlib.h>

enum { SBAR_WIDTH = 8 }; // width of scroll bar

void init_display(int argc, char ** argv, int iargc, char ** iargv);

bool is_console(void);

#endif//!INIT_DISPLAY_H
