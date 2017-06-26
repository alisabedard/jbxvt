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
	struct JBDim position;
	uint8_t length;
	bool is_double_width_line;
	const uint8_t __pad[2];
};
#endif//!JBXVT_JBXVTPAINTCONTEXT_H
