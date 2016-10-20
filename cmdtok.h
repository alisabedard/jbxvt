// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_CMDTOK_H
#define JBXVT_CMDTOK_H
#include "Token.h"
void jbxvt_get_token(struct Token * restrict tk);
int_fast16_t jbxvt_pop_char(const uint8_t flags)
	__attribute__((hot));
#endif//!JBXVT_CMDTOK_H
