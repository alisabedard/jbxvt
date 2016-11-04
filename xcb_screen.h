// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_XCB_SCREEN_H
#define JBXVT_XCB_SCREEN_H
#include <xcb/xcb.h>
// Get default colormap for the screen
xcb_colormap_t jbxvt_get_colormap(xcb_connection_t * xc);
// Get the root window of the screen
xcb_window_t jbxvt_get_root_window(xcb_connection_t * xc);
#endif//!JBXVT_XCB_SCREEN_H
