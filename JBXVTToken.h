/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_JBXVTTOKEN_H
#define JBXVT_JBXVTTOKEN_H
#include <stdint.h>
#include "JBXVTTokenType.h"
enum { JBXVT_TOKEN_MAX_LENGTH = 63, JBXVT_TOKEN_MAX_ARGS = 8 };
/*  Structure used to represent a piece of input from the program
 *  or an interesting X event.  */
struct JBXVTToken{
	// numeric arguments:
	int16_t arg[JBXVT_TOKEN_MAX_ARGS];
	// text for string tokens:
	uint8_t string[JBXVT_TOKEN_MAX_LENGTH + 1];
	enum JBXVTTokenType type;
	union {
		uint8_t private;	// non zero for priv ctl sequences
		uint8_t nlcount;	// number of newlines in the string
	};
	uint16_t length:12;		// length of string
	uint8_t nargs:4;		// number of arguments passed
	uint8_t tk_char;		// single (unprintable) character
};
#endif//!JBXVT_JBXVTTOKEN_H
