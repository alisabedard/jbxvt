#include "color.h"

#include "jbxvt.h"

#include <stdlib.h>
#include <xcb/xproto.h>

// returns pixel value for specified color
__attribute__((nonnull))
pixel_t get_pixel(const char * restrict color)
{
	return jb_get_pixel(jbxvt.X.xcb, jbxvt.X.screen->default_colormap,
		color);
}

// Use rgb color
pixel_t get_pixel_rgb(const uint16_t r, const uint16_t g, const uint16_t b)
{
	return jb_get_rgb_pixel(jbxvt.X.xcb,
		jbxvt.X.screen->default_colormap, r, g, b);
}

pixel_t set_color(const uint32_t vm, const pixel_t p, const xcb_gcontext_t gc)
{
	xcb_change_gc(jbxvt.X.xcb, gc, vm, (uint32_t[]){p});
	*(vm & XCB_GC_FOREGROUND ? &jbxvt.X.color.current_fg
		: &jbxvt.X.color.current_bg) = p;
	return p;
}

void set_fg_or_bg(const char * color, const bool is_fg)
{
	set_color(is_fg ? XCB_GC_FOREGROUND : XCB_GC_BACKGROUND,
		color ? get_pixel(color) : is_fg ? jbxvt.X.color.fg
		: jbxvt.X.color.bg, jbxvt.X.gc.tx);
}

