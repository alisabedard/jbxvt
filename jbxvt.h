#ifndef JBXVT_H
#define JBXVT_H

#include "color.h"
#include "command.h"
#include "Dim.h"
#include "screenst.h"
#include "selst.h"
#include "slinest.h"

// Use for all file descriptors:
typedef int fd_t;

struct JBXVT {
	struct {
		//Display * dpy;
		xcb_connection_t * xcb;
		xcb_screen_t * screen;
		int screen_number;
		xcb_font_t font;
		xcb_font_t bold_font;
		struct {
			xcb_window_t vt, sb, main;
		} win;
		struct {
			xcb_gcontext_t tx, hl, cu;
		} gc;
		struct {
			pixel_t bg, fg, cursor;
			pixel_t current_fg, current_bg;
		} color;
		int16_t font_ascent, font_descent;
		uint8_t font_height, font_width;
	} X;
	struct {
		struct screenst * current;
		struct screenst s1, s2;
		struct {
			struct slinest **data; // saved lines
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
		uint16_t send_count; // # chars waiting to be sent
	} com;
	struct {
		char *bg, *fg, *cu, *font, *bold_font, *display;
		int screen;
		bool show_scrollbar:1;
	} opt;
};

extern struct JBXVT jbxvt; // in jbxvt.c


// Print string to stderr
void jbputs(const char * restrict string);

#ifdef USE_LIKELY
#define likely(x)       __builtin_expect((x), true)
#define unlikely(x)     __builtin_expect((x), false)
#else//!USE_LIKELY
#define likely(x) (x)
#define unlikely(x) (x)
#endif//USE_LIKELY

#endif//!JBXVT_H
