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
#include "xeventst.h"

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
	uint16_t top, max;
};

struct JBXVTScreenData {
	VTScreen * current, * s;
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

struct JBXVTEventQueue {
	JBXVTEvent * start, * last;
};

struct JBXVTCommandData {
	struct JBXVTCommandContainer buf, stack;
	struct JBXVTEventQueue events;
	uint8_t * send_nxt; // next char to be sent
	long width; // # file descriptors being used
	fd_t fd; // file descriptor connected to the command
	fd_t xfd; // X connection file descriptor
	// type per sysconf(3):
	uint16_t send_count; // # chars waiting to be sent
};

enum CharacterSet{
	CHARSET_GB, CHARSET_ASCII, CHARSET_SG0, CHARSET_SG1, CHARSET_SG2
};

struct JBXVTPrivateModes {
	uint8_t charset[2];	// graphics mode char set
	uint8_t charsel:1;	// charset index
	bool att610:1;		// stop blinking cursor
	bool decanm:1;		// DECANM -- ANSI/VT52
	bool decawm:1;		// DECAWM auto-wrap flag
	bool deccolm:1;		// 132 column mode
	bool decdwl:1;		// double width line
	bool decom:1;		// origin mode flag
	bool decpff:1;		// DECPFF: print form feed
	bool decsclm:1;		// DECSCLM: slow scroll mode
	bool decscnm:1;		// DECSCNM: reverse-video mode
	bool dectcem:1;		// DECTCEM -- hide cursor
	bool gm52:1;		// VT52 graphics mode
	bool insert:1;		// insert mode flag
	bool mouse_x10:1;	// ptr coord on button press
	bool mouse_vt200:1;	// ptr press+release
	bool mouse_vt200hl:1;	// highlight tracking
	bool mouse_btn_evt:1;	// button event tracking
	bool mouse_any_evt:1;	// all motion tracking
	bool mouse_focus_evt:1; // focus tracking
	bool mouse_ext:1;	// UTF-8 coords
	bool mouse_sgr:1;	// sgr scheme
	bool mouse_urxvt:1;	// decimal integer coords
	bool mouse_alt_scroll:1;// send cursor up/down instead
	bool ptr_xy:1;		// send x y on button press/release
	bool ptr_cell:1;	// cell motion mouse tracking
	bool s8c1t:1;		// 7 or 8 bit controls
};

struct JBXVTOptionData {
	char *bg, *fg, *font, *bold_font, *display;
	Size size;
	int8_t screen;
	uint8_t elr; // DECELR
	bool show_scrollbar;
	uint8_t cursor_attr;
};

struct JBXVT {
	struct JBXVTXData X;
	struct JBXVTScreenData scr;
	struct JBXVTSelectionData sel;
	struct JBXVTCommandData com;
	struct JBXVTOptionData opt;
	struct JBXVTPrivateModes mode, saved_mode;
};

extern struct JBXVT jbxvt; // in jbxvt.c

#endif//!JBXVT_H
