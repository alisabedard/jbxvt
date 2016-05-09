#ifndef JBXVT_COLOR_H
#define JBXVT_COLOR_H

#include <stdint.h>

// Reset colors to stored background and foreground.
void reset_color(void);

void reset_fg(void);
void reset_bg(void);

void set_fg(const char * color); //foreground
void set_bg(const char * color); //background

#endif//!JBXVT_COLOR_H
