// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_CMDTOK_H
#define JBXVT_CMDTOK_H
#include <xcb/xcb.h>
struct JBXVTToken;
void jbxvt_get_token(xcb_connection_t * xc,
    struct JBXVTToken * tk);
// Initialize the static command buffer and stack
void jbxvt_init_cmdtok(void);
int16_t jbxvt_pop_char(xcb_connection_t * xc,
    const uint8_t flags) __attribute__((hot));
//  Push an input character back into the input queue.
void jbxvt_push_char(const uint8_t c);
#endif//!JBXVT_CMDTOK_H
