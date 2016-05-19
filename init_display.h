#ifndef INIT_DISPLAY_H
#define INIT_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include <X11/Xlib.h>

enum { SBAR_WIDTH = 12 }; // width of scroll bar

void init_display(uint8_t argc, char ** argv);

bool is_console(void);

#endif//!INIT_DISPLAY_H
