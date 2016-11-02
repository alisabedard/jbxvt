// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_DISPLAY_H
#define JBXVT_DISPLAY_H
#include "libjb/size.h"
#include <xcb/xcb.h>
xcb_connection_t * jbxvt_init_display(char * restrict name,
	struct JBDim size);
#endif//!JBXVT_DISPLAY_H
