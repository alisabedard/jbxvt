#ifndef JBXVT_HANDLE_SGR_H
#define JBXVT_HANDLE_SGR_H
#include <xcb/xcb.h>
#include "JBXVTToken.h"
void jbxvt_handle_sgr(xcb_connection_t * xc,
	struct JBXVTToken * restrict token)
	__attribute__((hot));
#endif//!JBXVT_HANDLE_SGR_H
