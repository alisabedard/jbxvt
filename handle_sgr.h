#ifndef HANDLE_SGR_H
#define HANDLE_SGR_H

#include "Token.h"

void handle_sgr(Token * restrict token)
	__attribute__((hot));

enum RenderFlag {
	RS_NONE = 0,
	RS_BOLD = 1,
	RS_ULINE = (1<<1),
	RS_BLINK = (1<<2),
	RS_RVID = (1<<3),
	RS_ITALIC = (1<<4),
	RS_INVISIBLE = (1<<5),
	// colors:
	// foreground: index or 9 bit octal rgb
	RS_F0 = (1<<6),
	RS_F1 = (1<<7),
	RS_F2 = (1<<8),
	RS_F3 = (1<<9),
	RS_F4 = (1<<10),
	RS_F5 = (1<<11),
	RS_F6 = (1<<12),
	RS_F7 = (1<<13),
	RS_F8 = (1<<14),
	// background: index or 9 bit octal rgb
	RS_B0 = (1<<15),
	RS_B1 = (1<<16),
	RS_B2 = (1<<17),
	RS_B3 = (1<<18),
	RS_B4 = (1<<19),
	RS_B5 = (1<<20),
	RS_B6 = (1<<21),
	RS_B7 = (1<<22),
	RS_B8 = (1<<23),
	// extended color support bits
	RS_FG_RGB = (1<<24),
	RS_BG_RGB = (1<<25),
	RS_FG_INDEX = (1<<26),
	RS_BG_INDEX = (1<<27),
};


#endif//!HANDLE_SGR_H
