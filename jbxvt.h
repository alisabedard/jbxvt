// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_H
#define JBXVT_H
#include "libjb/util.h"
#include "selection.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTSavedLine.h"
#include "JBXVTEvent.h"
#include "JBXVTScreen.h"
struct JBXVTScreenSavedLines {
	struct JBXVTSavedLine data[JBXVT_MAX_SCROLL]; // saved lines
	uint16_t top, max;
};
struct JBXVTScreenData {
	struct JBXVTScreen * current, * s;
	struct JBXVTScreenSavedLines sline;
	struct JBDim chars, pixels;
	uint32_t rstyle; // render style
	uint32_t saved_rstyle; // saved render style
	int16_t offset; // current vert saved line
};
struct JBXVTSelectionData {
	uint8_t * text;
	enum JBXVTSelectionUnit unit;
	struct JBDim end[3]; // end0, end1, anchor
	uint16_t length;
	bool type;
};
struct JBXVTCommandContainer {
	uint8_t *next, *top, *data;
};
struct JBXVTCommandData {
	struct JBXVTCommandContainer buf, stack;
	struct JBXVTEvent xev;
	uint8_t * send_nxt; // next char to be sent
	long width; // # file descriptors being used
	fd_t fd; // file descriptor connected to the command
	fd_t xfd; // X connection file descriptor
	pid_t pid; // command PID
	// type per sysconf(3):
	uint16_t send_count; // # chars waiting to be sent
};
enum CharacterSet{
	CHARSET_GB, CHARSET_ASCII, CHARSET_SG0, CHARSET_SG1, CHARSET_SG2
};
struct JBXVTOptionData {
	char *bg, *fg, *font, *bold_font, *italic_font, *display;
	struct JBDim size;
	int8_t screen;
	uint8_t elr; // DECELR
	bool show_scrollbar;
	uint8_t cursor_attr;
};
struct JBXVT {
	struct JBXVTScreenData scr;
	struct JBXVTSelectionData sel;
	struct JBXVTCommandData com;
	struct JBXVTOptionData opt;
	struct JBXVTPrivateModes mode;
};
extern struct JBXVT jbxvt; // in jbxvt.c
#endif//!JBXVT_H
