// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_TK_CHAR_H
#define JBXVT_TK_CHAR_H
#include <stdint.h>
#include <xcb/xcb.h>
void jbxvt_handle_tk_char(xcb_connection_t * xc, const uint8_t tk_char);
#endif//!JBXVT_TK_CHAR_H
