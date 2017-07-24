// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTKEYMAPS_H
#define JBXVT_JBXVTKEYMAPS_H
#include <xcb/xproto.h>
#include "JBXVTKeyStrings.h"
//  Structure used to map a keysym to a string.
struct JBXVTKeyMaps {
	xcb_keysym_t km_keysym;
	struct JBXVTKeyStrings km_normal;	/* The usual string */
	struct JBXVTKeyStrings km_alt;	/* The alternative string */
};
#endif//!JBXVT_JBXVTKEYMAPS_H
