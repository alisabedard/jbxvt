#ifndef CMDTOK_H
#define CMDTOK_H

#include "Token.h"

//  Special character returned by get_com_char().
enum ComCharReturn {
	GCC_NULL = 0x100, // Input buffer is empty
	ESC = 033
};

//  Flags used to control get_com_char();
enum ComCharFlags {BUF_ONLY=1, GET_XEVENTS=2};

void get_token(Token * restrict tk);

#endif//!CMDTOK_H
