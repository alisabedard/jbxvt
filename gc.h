// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_GC_H
#define JBXVT_GC_H
#include <xcb/xproto.h>
xcb_gcontext_t jbxvt_get_cursor_gc(xcb_connection_t * xc);
xcb_gcontext_t jbxvt_get_text_gc(xcb_connection_t * xc);
#endif//!JBXVT_GC_H
