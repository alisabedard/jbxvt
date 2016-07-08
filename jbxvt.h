#ifndef JBXVT_H
#define JBXVT_H

#include "color.h"
#include "command.h"
#include "Size.h"
#include "VTScreen.h"
#include "selst.h"
#include "SLine.h"

#define WARN_ERR	"Error: "
#define WARN_SIG	"Signal Received"
#define WARN_RES	"Resource unavailable: "
#define RES_DPY		"Bad DISPLAY"
#define RES_CMD		"Could not execute command"
#define RES_FNT		"Could not load font"
#define RES_SSN		"Could not open session"
#define RES_TTY		"Could not open tty"
#define RES_TMP		"Could not open utmp database"

// Use for all file descriptors:
typedef int fd_t;

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct {
	struct {
		//Display * dpy;
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
			pixel_t bg, fg, cursor;
			pixel_t current_fg, current_bg;
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
		Size size;
		bool show_scrollbar;
	} opt;
} JBXVT;

extern JBXVT jbxvt; // in jbxvt.c


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
