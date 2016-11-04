#ifndef JBXVT_HANDLE_SGR_H
#define JBXVT_HANDLE_SGR_H
#include "Token.h"
#include <xcb/xcb.h>
void jbxvt_handle_sgr(xcb_connection_t * xc,
	struct Token * restrict token)
	__attribute__((hot));
#endif//!JBXVT_HANDLE_SGR_H
