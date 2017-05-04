// Copyright 2017, Jeffrey E. Bedard
#include "gc.h"
#define DEFUN_GET_GC(name) xcb_gcontext_t name(xcb_connection_t * xc) \
{\
	static xcb_gcontext_t gc;\
	return gc ? gc : (gc = xcb_generate_id(xc));\
}
DEFUN_GET_GC(jbxvt_get_text_gc);
DEFUN_GET_GC(jbxvt_get_cursor_gc);
