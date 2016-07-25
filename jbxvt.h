#ifndef JBXVT_H
#define JBXVT_H

#include "command.h"
#include "libjb/util.h"
#include "libjb/xcb.h"
#include "selection.h"
#include "selend.h"
#include "Size.h"
#include "SLine.h"
#include "VTScreen.h"

struct JBXVTXWindows {
	xcb_window_t vt, sb, main;
};

struct JBXVTXGCs {
	xcb_gcontext_t tx, cu;
};

struct JBXVTXPixels {
	pixel_t bg, fg, current_fg, current_bg;
};

struct JBXVTFontData {
	xcb_font_t normal, bold;
	Size size;
	int8_t ascent;
};

struct JBXVTXData {
	xcb_connection_t * xcb;
	xcb_screen_t * screen;
	xcb_atom_t clipboard;
	struct JBXVTXWindows win;
	struct JBXVTXGCs gc;
	struct JBXVTXPixels color;
	struct JBXVTFontData f;
	Size window_size;
	int8_t screen_number;
};

struct JBXVTScreenSLine {
	SLine **data; // saved lines
	int32_t top;
	uint16_t max; // max # of saved lines
};

struct JBXVTScreenData {
	VTScreen * current, s[2];
	struct JBXVTScreenSLine sline;
	Size pixels, chars;
	uint32_t rstyle; // render style
	uint32_t saved_rstyle; // saved render style
	int16_t offset; // current vert saved line
};

struct JBXVTSelectionData {
	uint8_t * text;
	SelEnd end[2], // selection endpoints
	       anchor; //selection anchor
	enum selunit unit;
	uint16_t length;
};

struct JBXVTCommandContainer {
	uint8_t *next, *top, *data;
};

struct JBXVTCommandData {
	struct JBXVTCommandContainer buf, stack;
	uint8_t * send_nxt; // next char to be sent
	long width; // # file descriptors being used
	fd_t fd; // file descriptor connected to the command
	// type per sysconf(3):
	uint16_t send_count; // # chars waiting to be sent
};

struct JBXVTOptionData {
	char *bg, *fg, *font, *bold_font, *display;
	Size size;
	int8_t screen;
	bool show_scrollbar;
};

typedef struct {
	struct JBXVTXData X;
	struct JBXVTScreenData scr;
	struct JBXVTSelectionData sel;
	struct JBXVTCommandData com;
	struct JBXVTOptionData opt;
} JBXVT;

extern JBXVT jbxvt; // in jbxvt.c

#endif//!JBXVT_H
