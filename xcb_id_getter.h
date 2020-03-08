// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_XCB_ID_GETTER_H
#define JBXVT_XCB_ID_GETTER_H
#include <xcb/xcb.h>
#define XCB_ID_GETTER(name) uint32_t name(xcb_connection_t * xc) \
{\
    static uint32_t id;\
    return id ? id : (id = xcb_generate_id(xc));\
}
#endif//!JBXVT_XCB_ID_GETTER_H
