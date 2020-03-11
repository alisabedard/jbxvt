// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_DISPLAY_H
#define JBXVT_DISPLAY_H
#include <xcb/xcb.h>
struct JBXVTOptions;
xcb_connection_t * jbxvt_init_display(char * name,
    struct JBXVTOptions * opt);
#endif//!JBXVT_DISPLAY_H
