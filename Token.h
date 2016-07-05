/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef TOKENST_H
#define TOKENST_H

#include "TokenType.h"

#include <stdint.h>

enum TokenLimits {
	TKS_MAX =	63,	// max length of a string token
	TK_MAX_ARGS =	8	// max # of numeric arguments
};

//  Values of tk_region for Xevent generated tokens.
typedef enum {
	REGION_NONE, REGION_MAINWIN, REGION_SCREEN, REGION_SCROLLBAR
} JBXVTRegion;

/*  Structure used to represent a piece of input from the program
 *  or an interesting X event.  */
typedef struct {
	int16_t arg[TK_MAX_ARGS];	// first numerical arguments
	uint8_t string[TKS_MAX + 1];	// the text for string tokens
	TokenType type;			// type of token
	JBXVTRegion region;		// where token applies
	uint16_t private;		// non zero for priv ctl sequences
	uint16_t nlcount;		// number of newlines in the string
	uint16_t length;		// length of string
	uint8_t tk_char;		// single (unprintable) character
	uint8_t nargs:4;		// number of arguments passed
} Token;

#endif//!TOKENST_H
