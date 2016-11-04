// Copyright 2016, Jeffrey E. Bedard
#include "mouse.h"
#include "cmdtok.h"
#include "command.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "mode.h"
#include "size.h"
static uint8_t get_mod(const uint16_t state)
{
	// 4=Shift, 8=Meta, 16=Control
	uint8_t mod = 0;
	if (state & XCB_KEY_BUT_MASK_SHIFT)
		mod += 4;
	if (state & XCB_KEY_BUT_MASK_MOD_1)
		mod += 8;
	if (state & XCB_KEY_BUT_MASK_CONTROL)
		mod += 16;
	return mod;
}
static bool track_mouse_sgr(uint8_t b, struct JBDim p, const bool rel)
{
	if (!jbxvt_get_modes()->mouse_sgr)
		return false;
	dprintf(jbxvt_get_fd(), "%s<%c;%c;%c%c", jbxvt_get_csi(),
		b, p.x, p.y, rel ? 'm' : 'M');
	LOG("(SGR): <%c;%c;%c%c", b, p.x, p.y, rel ? 'm' : 'M');
	return true;
}
static void locator_report(const uint8_t b, struct JBDim p)
{
	if (!jbxvt_get_modes()->elr)
		return;
	if (jbxvt_get_modes()->elr_once)
		jbxvt_get_modes()->elr_once = jbxvt_get_modes()->elr = false;
	if (jbxvt_get_modes()->elr_pixels)
		p = jbxvt_chars_to_pixels(p);
	// DECLRP
	dprintf(jbxvt_get_fd(), "%s%d;%d;%d;%d;0&w", jbxvt_get_csi(),
		b * 2, 7, p.y, p.x);
}
static uint8_t get_b(uint8_t b, const uint32_t state)
{
	// base button on 0:
	--b;
	// add modifiers:
	return b + get_mod(state);
}
static void track_mouse_x10(uint8_t b, struct JBDim p)
{
	// X10 encoding:
	b += 32;
	p.x += 32;
	p.y += 32;
	dprintf(jbxvt_get_fd(), jbxvt_get_modes()->mouse_urxvt ? "%s%d;%d;%dM"
		: "%sM%c%c%c", jbxvt_get_csi(), b, p.x, p.y);
	LOG(jbxvt_get_modes()->mouse_urxvt ? "%d;%d;%dM" : "M%c%c%c", b, p.x, p.y);
}
void jbxvt_track_mouse(uint8_t b, uint32_t state, struct JBDim p,
	const uint8_t flags)
{
	LOG("track_mouse(b=%d, p={%d, %d})", b, p.x, p.y);
	// get character position:
	p = jbxvt_pixels_to_chars(p);
	// modify for a 1-based row/column system
	++p.x; ++p.y;
	const bool wheel = b == 4 || b == 5;
	locator_report(b, p);
	// Release handling:
	if (flags & JBXVT_RELEASE) {
		if (jbxvt_get_modes()->mouse_x10 || wheel) // wheel release untracked
			return; // release untracked in x10 mode
		LOG("TRACK_RELEASE");
		if (!jbxvt_get_modes()->mouse_sgr) // sgr reports which button was released
			b = 3; // release code
	} else if (wheel) { // wheel release untracked
		b += 60; // Wheel mouse handling
		LOG("wheel b: %d", b);
	} else
		b = get_b(b, state);
	if (!track_mouse_sgr(b, p, flags & JBXVT_RELEASE))
		track_mouse_x10(b, p);
}
#define TRK(it) jbxvt_get_modes()->mouse_##it
bool jbxvt_get_mouse_motion_tracked(void)
{
	return TRK(btn_evt) || TRK(any_evt);
}
bool jbxvt_get_mouse_tracked(void)
{
	return TRK(x10) || TRK(vt200) || TRK(vt200hl) || TRK(ext)
		|| TRK(sgr) || TRK(urxvt) || jbxvt_get_mouse_motion_tracked();
}
