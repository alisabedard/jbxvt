// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_DISPLAY_H
#define JBXVT_DISPLAY_H
#include <xcb/xcb.h>
#include "JBXVTOptions.h"
xcb_connection_t * jbxvt_init_display(char * restrict name,
	struct JBXVTOptions * restrict opt);
#endif//!JBXVT_DISPLAY_H
