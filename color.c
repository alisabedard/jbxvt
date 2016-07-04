#include "color.h"

#include "jbxvt.h"

#include <stdlib.h>
#include <xcb/xproto.h>

// returns pixel value for specified color
__attribute__((nonnull))
pixel_t get_pixel(const char * restrict color)
{
	register uint_fast8_t l = 0;
	while(color[++l]);
	xcb_alloc_named_color_cookie_t c = xcb_alloc_named_color(jbxvt.X.xcb,
		jbxvt.X.screen->default_colormap, l, color);
	xcb_alloc_named_color_reply_t * r
		= xcb_alloc_named_color_reply(jbxvt.X.xcb, c, NULL);
	if(!r) return 0;
	pixel_t p = r->pixel;
	free(r);
	return p;
}

// Use rgb color
pixel_t get_pixel_rgb(int16_t r, int16_t g, int16_t b)
{
	xcb_alloc_color_cookie_t c = xcb_alloc_color(jbxvt.X.xcb,
		jbxvt.X.screen->default_colormap, r, g, b);
	xcb_alloc_color_reply_t * rpl = xcb_alloc_color_reply(jbxvt.X.xcb,
		c, NULL);
	pixel_t p = rpl->pixel;
	free(rpl);
	return p;
}

pixel_t set_color(const uint32_t vm, const pixel_t p, const xcb_gcontext_t gc)
{
	xcb_change_gc(jbxvt.X.xcb, gc, vm, (uint32_t[]){p});
	*(vm & XCB_GC_FOREGROUND ? &jbxvt.X.color.current_fg
		: &jbxvt.X.color.current_bg) = p;
	return p;
}

void set_fg(const char * restrict color)
{
	jbxvt.X.color.current_fg = set_color(XCB_GC_FOREGROUND,
		color?get_pixel(color):jbxvt.X.color.fg, jbxvt.X.gc.tx);
}

void set_bg(const char * restrict color)
{
	jbxvt.X.color.current_bg = set_color(XCB_GC_BACKGROUND,
		color?get_pixel(color):jbxvt.X.color.bg, jbxvt.X.gc.tx);
}

