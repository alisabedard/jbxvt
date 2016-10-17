// Copyright 2016, Jeffrey E. Bedard
#ifndef CMDTOK_H
#define CMDTOK_H
#include "Token.h"
void get_token(struct Token * restrict tk);
int_fast16_t get_com_char(const uint8_t flags)
	__attribute__((hot));
#endif//!CMDTOK_H
