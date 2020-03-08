// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTKEYMAPS_H
#define JBXVT_JBXVTKEYMAPS_H
#include <xcb/xproto.h>
//  Structure used to map a keysym to a string.
struct JBXVTKeyMaps {
    xcb_keysym_t km_keysym;
    /* The usual string and alternative strings associated with this
     keycode follow.  The first element of each array is the format string
     selector per enum JBXVTKeySymType.  The second element of each array
     is the value used in creating the returned string (as passed through
     the format string).  */
    uint8_t km_normal[2], km_alt[2];
};
#endif//!JBXVT_JBXVTKEYMAPS_H
