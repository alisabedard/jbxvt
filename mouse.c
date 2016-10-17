// Copyright 2016, Jeffrey E. Bedard
#include "mouse.h"
#include "command.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "screen.h"
#include <stdlib.h>
#include <string.h>
static uint8_t get_mod(const uint16_t state)
{
	// 4=Shift, 8=Meta, 16=Control
	uint8_t mod = 0;
#define MOD(mk, n) if(state&XCB_KEY_BUT_MASK_##mk) mod+=n;
	MOD(SHIFT, 4); MOD(MOD_1, 8); MOD(CONTROL, 16);
	return mod;
}
#define MD jbxvt.mode
static bool track_mouse_sgr(uint8_t b, struct JBDim p, const bool rel)
{
	if (!MD.mouse_sgr)
		return false;
	cprintf("\033[<%c;%c;%c%c", b, p.x, p.y, rel ? 'm' : 'M');
	return true;
}
static void locator_report(const uint8_t b, struct JBDim p)
{
	if (!MD.elr)
		return;
	if (MD.elr_once)
		MD.elr_once = MD.elr = false;
	if (MD.elr_pixels)
		p = jbxvt_get_pixel_size(p);
	// DECLRP
	cprintf("\033[%d;%d;%d;%d;0&w", b * 2, 7, p.y, p.x);
}
void jbxvt_track_mouse(uint8_t b, uint32_t state, struct JBDim p,
	const uint8_t flags)
{
	LOG("track_mouse(b=%d, p={%d, %d})", b, p.x, p.y);
	// get character position:
	p = jbxvt_get_char_size(p);
	// modify for a 1-based row/column system
	++p.x; ++p.y;
	const bool wheel = b == 4 || b == 5;
	locator_report(b, p);
	struct JBXVTPrivateModes * m = &jbxvt.mode;
	// Release handling:
	if (flags & JBXVT_RELEASE) {
		if (m->mouse_x10 || wheel) // wheel release untracked
			return; // release untracked in x10 mode
		LOG("TRACK_RELEASE");
		if (!m->mouse_sgr) // sgr reports which button was released
			b = 4; // release code, -1 later
	} else if (wheel) { // wheel release untracked
		b += 65; // Wheel mouse handling
		LOG("wheel b: %d", b);
	}
	// base button on 0:
	--b;
	// add modifiers:
	b += get_mod(state);
	if (track_mouse_sgr(b, p, flags & JBXVT_RELEASE)) {
		LOG("mouse_sgr");
		return;
	}
	// X10 encoding:
	b += 32;
	p.x += 32;
	p.y += 32;
	char * format = m->mouse_urxvt ? "\033[%d;%d;%dM" : "\033[M%c%c%c";
#ifndef DEBUG
	cprintf(format, b, p.x, p.y);
#else//DEBUG
	char * out = strdup(cprintf(format, b, p.x, p.y));
	for(char * i = out; *i; ++i)
		if (*i == '\033')
			*i = 'E';
	LOG("track_mouse: %s\n", out);
	free(out);
#endif//!DEBUG
}
#define TRK(it) jbxvt.mode.mouse_##it
bool jbxvt_get_mouse_motion_tracked(void)
{
	return TRK(btn_evt) || TRK(any_evt);
}
bool jbxvt_get_mouse_tracked(void)
{
	return TRK(x10) || TRK(vt200) || TRK(vt200hl) || TRK(ext)
		|| TRK(sgr) || TRK(urxvt) || jbxvt_get_mouse_motion_tracked();
}
