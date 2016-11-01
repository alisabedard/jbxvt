// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_CMDTOK_H
#define JBXVT_CMDTOK_H
#include "Token.h"
#include <xcb/xcb.h>
void jbxvt_get_token(xcb_connection_t * xc,
	struct Token * restrict tk);
int_fast16_t jbxvt_pop_char(xcb_connection_t * xc,
	const uint8_t flags) __attribute__((hot));
const char * jbxvt_get_csi(void);
#endif//!JBXVT_CMDTOK_H
