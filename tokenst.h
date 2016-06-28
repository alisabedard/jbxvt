#ifndef TOKENST_H
#define TOKENST_H

#include <stdint.h>

enum TokenLimits {
	TKS_MAX =	63,	// max length of a string token
	TK_MAX_ARGS =	8	// max # of numeric arguments
};

/*  Structure used to represent a piece of input from the program
 *  or an interesting X event.
 */
typedef struct tokenst {
	int16_t tk_arg[TK_MAX_ARGS];	/* first numerical arguments */
	uint8_t tk_string[TKS_MAX + 1];	/* the text for string tokens */
	uint16_t tk_private;		/* non zero for private control sequences */
	uint16_t tk_nlcount;		/* number of newlines in the string */
	uint16_t tk_type;		/* the token type */
	uint16_t tk_length;		/* length of string */
	uint8_t tk_char;		/* single (unprintable) character */
	uint8_t tk_nargs:4;
	uint8_t tk_region:2; // terminal or scrollbar
} Token;

#endif//!TOKENST_H
