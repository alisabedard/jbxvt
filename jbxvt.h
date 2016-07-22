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

typedef struct {
	struct {
		xcb_connection_t * xcb;
		xcb_screen_t * screen;
		int screen_number;
		xcb_font_t font;
		xcb_font_t bold_font;
		xcb_atom_t clipboard;
		struct {
			xcb_window_t vt, sb, main;
		} win;
		struct {
			xcb_gcontext_t tx, cu;
		} gc;
		struct {
			pixel_t bg, fg, current_fg, current_bg;
		} color;
		Size font_size;
		Size window_size;
		int16_t font_ascent;
	} X;
	struct {
		VTScreen * current;
		VTScreen s1, s2;
		struct {
			SLine **data; // saved lines
			int32_t top;
			uint16_t max; // max # of saved lines
		} sline;
		Size pixels, chars;
		uint32_t rstyle; // render style
		uint32_t saved_rstyle; // saved render style
		int16_t offset; // current vert saved line
	} scr;
	struct {
		uint8_t * text;
		SelEnd end1, end2, // selection endpoints
			     anchor; //selection anchor
		enum selunit unit;
		uint16_t length;
	} sel;
	struct {
		uint8_t * send_nxt; // next char to be sent
		fd_t fd; // file descriptor connected to the command
		// type per sysconf(3):
		long width; // # file descriptors being used
		struct {
			uint8_t *next, *top;
			uint8_t data[COM_BUF_SIZE];
		} buf;
		struct {
			uint8_t *top;
			uint8_t data[COM_PUSH_MAX];
		} stack;
		uint16_t send_count; // # chars waiting to be sent
	} com;
	struct {
		char *bg, *fg, *font, *bold_font, *display;
		int screen;
		Size size;
		bool show_scrollbar;
	} opt;
} JBXVT;

extern JBXVT jbxvt; // in jbxvt.c

#endif//!JBXVT_H
