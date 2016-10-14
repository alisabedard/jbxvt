#ifndef HANDLE_SGR_H
#define HANDLE_SGR_H

#include "Token.h"

void handle_sgr(struct Token * restrict token)
	__attribute__((hot));

enum RenderFlag {
	JBXVT_RS_NONE = 0,
	JBXVT_RS_BOLD = 1,
	JBXVT_RS_ULINE = (1<<1),
	JBXVT_RS_BLINK = (1<<2),
	JBXVT_RS_RVID = (1<<3),
	JBXVT_RS_ITALIC = (1<<4),
	JBXVT_RS_INVISIBLE = (1<<5),
	// colors:
	// foreground: index or 9 bit octal rgb
	JBXVT_RS_F0 = (1<<6),
	JBXVT_RS_F1 = (1<<7),
	JBXVT_RS_F2 = (1<<8),
	JBXVT_RS_F3 = (1<<9),
	JBXVT_RS_F4 = (1<<10),
	JBXVT_RS_F5 = (1<<11),
	JBXVT_RS_F6 = (1<<12),
	JBXVT_RS_F7 = (1<<13),
	JBXVT_RS_F8 = (1<<14),
	// background: index or 9 bit octal rgb
	JBXVT_RS_B0 = (1<<15),
	JBXVT_RS_B1 = (1<<16),
	JBXVT_RS_B2 = (1<<17),
	JBXVT_RS_B3 = (1<<18),
	JBXVT_RS_B4 = (1<<19),
	JBXVT_RS_B5 = (1<<20),
	JBXVT_RS_B6 = (1<<21),
	JBXVT_RS_B7 = (1<<22),
	JBXVT_RS_B8 = (1<<23),
	// extended color support bits
	JBXVT_RS_FG_RGB = (1<<28),
	JBXVT_RS_BG_RGB = (1<<29),
	JBXVT_RS_FG_INDEX = (1<<30),
	JBXVT_RS_BG_INDEX = (1<<31),
};


#endif//!HANDLE_SGR_H
