/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SCREEN_H
#define SCREEN_H

#include "screenst.h"

#include <stdint.h>
#include <X11/Xlib.h>

//  flags for scr_move()
enum ScrMoveFlags {
	COL_RELATIVE = 1, // column movement is relative
	ROW_RELATIVE = 2  // row movement is relative
};

enum {
	MAX_SCROLL = 64 // max # lines that can scroll at once
};

//  arguments to the screen delete functions
enum ScrDelArg { END, START, ENTIRE };

//  rendition style flags:
#define RS_NONE	0		/* Normal */
#define RS_BOLD		1/* Bold face */
#define RS_ULINE	(1<<1)	/* underline */
#define RS_BLINK	(1<<2)	/* blinking */
#define RS_RVID		(1<<3) /* reverse video */
#define RS_ITALIC	(1<<4)
#define RS_LOW		(1<<5) // low intensity
// For colors:
// foreground:
#define RS_F0		(1<<6)
#define RS_F1		(1<<7)
#define RS_F2		(1<<8)
#define RS_F3		(1<<9)
#define RS_F4		(1<<10)
#define RS_F5		(1<<11)
#define RS_F6		(1<<12)
#define RS_F7		(1<<13)
#define RS_FR		(1<<14) // reset
// background:
#define RS_B0		(1<<15)
#define RS_B1		(1<<16)
#define RS_B2		(1<<17)
#define RS_B3		(1<<18)
#define RS_B4		(1<<19)
#define RS_B5		(1<<20)
#define RS_B6		(1<<21)
#define RS_B7		(1<<22)
#define RS_BR		(1<<23) // reset

#define RS_BF		(1<<24) // bright foreground
#define RS_BB		(1<<25) // bright background

// not enough bits for bright backgrounds

// Normal colors:
#define COLOR_0 "black"
#define COLOR_1 "red3"
#define COLOR_2 "green3"
#define COLOR_3 "yellow3"
#define COLOR_4 "blue3"
#define COLOR_5 "magenta3"
#define COLOR_6 "cyan3"
#define COLOR_7 "grey90"

// Bright colors:
#define BCOLOR_0 "grey10"
#define BCOLOR_1 "red3"
#define BCOLOR_2 "green3"
#define BCOLOR_3 "yellow3"
#define BCOLOR_4 "blue3"
#define BCOLOR_5 "magenta3"
#define BCOLOR_6 "cyan3"
#define BCOLOR_7 "white"

void home_screen(void);
int16_t is_string_char(int16_t c);
void scr_change_rendition(const uint32_t style);
void scr_change_screen(const uint8_t direction);
void scr_delete_lines(uint8_t count);

//  Return the width and height of the screen.
void scr_get_size(uint16_t * restrict width_p,
	uint16_t * restrict height_p);

void scr_index(void);
void scr_init(void);
void scr_insert_lines(int8_t count);
void scr_move_by(const int16_t y);
void scr_move_to(int16_t y);
void scr_report_display(void);
void scr_report_position(void);
void scr_restore_cursor(void);
void scr_rindex(void);
void scr_save_cursor(void);
void scr_set_margins(int16_t top, int16_t bottom);

#endif//!SCREEN_H
