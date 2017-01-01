// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_MOUSE_H
#define JBXVT_MOUSE_H
#include <stdbool.h>
#include <stdint.h>
struct JBDim;
enum { JBXVT_RELEASE = 1, JBXVT_MOTION = 2};
bool jbxvt_get_mouse_tracked(void);
bool jbxvt_get_mouse_motion_tracked(void);
void jbxvt_track_mouse(uint8_t b, uint32_t state, struct JBDim p,
	const uint8_t flags);
#endif//!JBXVT_MOUSE_H
