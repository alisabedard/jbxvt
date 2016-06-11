/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREEN_H
#define SCREEN_H

#include "jbxvt.h"
#include "screenst.h"

//  flags for scr_move()
enum ScrMoveFlags {
	COL_RELATIVE = 1, // column movement is relative
	ROW_RELATIVE = 2  // row movement is relative
};

enum {
	MAX_SCROLL = 24 /* max # lines that can scroll at once.
			    126 is greatest value supported. */
};

//  arguments to the screen delete functions
enum ScrDelArg { END, START, ENTIRE };

enum RenditionStyleFlags {
	RS_NONE = 0,
	RS_BOLD = 1,
	RS_ULINE = (1<<1),
	RS_BLINK = (1<<2),
	RS_RVID = (1<<3),
	RS_ITALIC = (1<<4),
	RS_LOW = (1<<5),
	// colors:
	// foreground:
	RS_F0 = (1<<6),
	RS_F1 = (1<<7),
	RS_F2 = (1<<8),
	RS_F3 = (1<<9),
	RS_F4 = (1<<10),
	RS_F5 = (1<<11),
	RS_F6 = (1<<12),
	RS_F7 = (1<<13),
	RS_FR = (1<<14),
	// background:
	RS_B0 = (1<<15),
	RS_B1 = (1<<16),
	RS_B2 = (1<<17),
	RS_B3 = (1<<18),
	RS_B4 = (1<<19),
	RS_B5 = (1<<20),
	RS_B6 = (1<<21),
	RS_B7 = (1<<22),
	RS_BR = (1<<23),
	// bright foreground: masks above colors
	RS_BF = (1<<24),
	// bright background: masks above colors
	RS_BB = (1<<25),
	// extended color support bits
	RS_FG_RGB = (1<<26),
	RS_BG_RGB = (1<<27),
	RS_FG_INDEX = (1<<28),
	RS_BG_INDEX = (1<<29),
};

// Normal colors:
#define COLOR_0 "black"
#define COLOR_1 "red2"
#define COLOR_2 "green2"
#define COLOR_3 "yellow2"
#define COLOR_4 "blue2"
#define COLOR_5 "magenta2"
#define COLOR_6 "cyan2"
#define COLOR_7 "white"

// Bright colors:
#define BCOLOR_0 "grey10"
#define BCOLOR_1 "pink"
#define BCOLOR_2 "lime"
#define BCOLOR_3 "lightyellow"
#define BCOLOR_4 "skyblue"
#define BCOLOR_5 "violet"
#define BCOLOR_6 "aquamarine"
#define BCOLOR_7 "white"

void home_screen(void);

//  Change the rendition style.
void scr_style(const uint32_t style);

//  Change between the alternate and the main screens
//  mode_high is true for screen 2
void scr_change_screen(const bool mode_high);

//  Delete count lines and scroll up the bottom of the screen to fill the gap
void scr_delete_lines(const uint8_t count);

/* Move the cursor up if mod is positive or down if mod is negative,
   by mod number of lines and scroll if necessary.  */
void scr_index_by(const int8_t mod);
#define scr_index() scr_index_by(1)
#define scr_rindex() scr_index_by(-1)
/*  Perform any initialisation on the screen data structures.  Called just once
 *  at startup. */
void scr_init(void);

/*  Insert count blank lines at the current position and scroll the lower lines
 *  down.  */
void scr_insert_lines(const int8_t count);

/*  Move the display so that line represented by scrollbar value y is at the top
 *  of the screen.  */
void scr_move_to(int16_t y);

//  Send the name of the current display to the command.
void scr_report_display(void);

#endif//!SCREEN_H
