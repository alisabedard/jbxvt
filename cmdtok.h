// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_CMDTOK_H
#define JBXVT_CMDTOK_H
#include <stdint.h>
#include <xcb/xcb.h>
struct JBXVTToken;
void jbxvt_get_token(xcb_connection_t * xc,
	struct JBXVTToken * restrict tk);
// Initialize the static command buffer and stack
void jbxvt_init_cmdtok(void);
int_fast16_t jbxvt_pop_char(xcb_connection_t * xc,
	const uint8_t flags) __attribute__((hot));
//  Push an input character back into the input queue.
void jbxvt_push_char(const uint8_t c);
const char * jbxvt_get_csi(void);
#endif//!JBXVT_CMDTOK_H
