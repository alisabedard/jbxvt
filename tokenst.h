#ifndef TOKENST_H
#define TOKENST_H

#include <stdint.h>

enum TokenLimits {
	TKS_MAX =	128,	// max length of a string token
	TK_MAX_ARGS =	8	// max # of numeric arguments
};

/*  Structure used to represent a piece of input from the program
 *  or an interesting X event.
 */
struct tokenst {
	uint16_t tk_type;		/* the token type */
	int32_t tk_private;		/* non zero for private control sequences */
	unsigned char tk_char;		/* single (unprintable) character */
	unsigned char tk_string[TKS_MAX + 1];/* the text for string tokens */
	int16_t tk_nlcount;		/* number of newlines in the string */
	uint16_t tk_length;		/* length of string */
	int32_t tk_arg[TK_MAX_ARGS];/* first two numerical arguments */
	int8_t tk_nargs:4;		/* number of numerical arguments */
	int8_t tk_region:4;		/* terminal or scrollbar */
};

#endif//!TOKENST_H
