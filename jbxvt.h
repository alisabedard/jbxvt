#ifndef JBXVT_H
#define JBXVT_H

#include "selst.h"

#include <stdbool.h>
#include <stdint.h>
#include <X11/Xlib.h>

struct JBXVT {
	struct {
		uint8_t font_height, font_width;
		Display * dpy;
		XFontStruct *font;
		struct {
			Window vt, sb, main;
		} win;
		struct {
			GC tx, ne, hl, cu, sb;
		} gc;
		struct {
			Colormap map;
			unsigned long bg, fg, cursor;
		} color;
	} X;
	struct {
		int32_t offset; // current vert saved line
		uint32_t rstyle; // render style
	} scr;
	struct {
		struct selst end1, end2, // selection endpoints
			     anchor; //selection anchor
	} sel;
	struct {
		bool save_rstyle:1;
	} opt;
};

extern struct JBXVT jbxvt; // in xvt.c

#endif//!JBXVT_H
