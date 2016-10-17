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
/*  Structure used to represent a piece of input from the program
 *  or an interesting X event.  */
struct Token{
	int32_t arg[TK_MAX_ARGS];	// first numerical arguments
	uint8_t string[TKS_MAX + 1];	// the text for string tokens
	TokenType type;			// type of token
	uint16_t private;		// non zero for priv ctl sequences
	uint16_t nlcount;		// number of newlines in the string
	uint16_t length:12;		// length of string
	uint8_t nargs:4;		// number of arguments passed
	uint8_t tk_char;		// single (unprintable) character
};
#endif//!TOKENST_H
