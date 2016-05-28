#ifndef JBXVT_H
#define JBXVT_H

#include "color.h"
#include "command.h"
#include "Dim.h"
#include "screenst.h"
#include "selst.h"
#include "slinest.h"

#include <stdbool.h>
#include <stdint.h>
#include <X11/Xlib.h>

// Use for all file descriptors:
typedef int fd_t;

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
			pixel_t bg, fg, cursor;
		} color;
		uint8_t font_height, font_width, screen;
	} X;
	struct {
		struct screenst * current;
		uint32_t rstyle; // render style
		uint32_t saved_rstyle; // saved render style
		struct screenst s1, s2;
		struct {
			struct slinest **data; // saved lines
			uint16_t max; // max # of saved lines
			uint16_t top; /* high water mark
					       of saved scroll lines */
		} sline;
		Size pixels, chars;
		int16_t offset; // current vert saved line
	} scr;
	struct {
		uint8_t * text;
		struct selst end1, end2, // selection endpoints
			     anchor; //selection anchor
		uint16_t length;
	} sel;
	struct {
		uint8_t * send_nxt; // next char to be sent
		fd_t fd; // file descriptor connected to the command
		fd_t width; // # file descriptors being used
		struct {
			uint8_t *next, *top;
			uint8_t data[COM_BUF_SIZE];
		} buf;
		struct {
			uint8_t *top;
			uint8_t data[COM_PUSH_MAX];
		} stack;
		int8_t send_count; // # chars waiting to be sent
	} com;
	struct {
		char *bg, *fg, *cu, *font;
		bool show_scrollbar:1;
	} opt;
};

extern struct JBXVT jbxvt; // in jbxvt.c

// constrain rc between 0 and lim, return new value
// use more generic int type as this is used in various places.
unsigned int constrain(const int rc, const int lim)
	__attribute__((const));

#endif//!JBXVT_H
