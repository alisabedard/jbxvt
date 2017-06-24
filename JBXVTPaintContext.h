// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTPAINTCONTEXT_H
#define JBXVT_JBXVTPAINTCONTEXT_H
#include <stdbool.h>
#include <xcb/xcb.h>
#include "libjb/JBDim.h"
#include "rstyle.h"
struct JBXVTPaintContext {
	xcb_connection_t * xc;
	uint8_t * string;
	rstyle_t * style;
	uint16_t length;
	struct JBDim position;
	bool is_double_width_line;
	const uint8_t pad;
};
#endif//!JBXVT_JBXVTPAINTCONTEXT_H
