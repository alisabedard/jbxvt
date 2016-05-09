#ifndef JBXVT_H
#define JBXVT_H

#include <X11/Xlib.h>

struct JBXVT {
	struct {
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
};

extern struct JBXVT jbxvt; // in xvt.c

#endif//!JBXVT_H
