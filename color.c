#include "color.h"

#include "jbxvt.h"

#include <stdlib.h>
#include <xcb/xproto.h>

// returns pixel value for specified color
__attribute__((nonnull))
pixel_t get_pixel(const char * restrict color)
{
	uint8_t l = 0;
	while(color[++l]);
	xcb_alloc_named_color_cookie_t c = xcb_alloc_named_color(
		jbxvt.X.xcb, jbxvt.X.color.map, l, color);
	xcb_alloc_named_color_reply_t * r
		= xcb_alloc_named_color_reply(
		jbxvt.X.xcb, c, NULL);
	pixel_t p = r->pixel;
	free(r);
	return p;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((regparm(3)))
#endif//x86
static inline pixel_t set_color(const unsigned long vm,
	const pixel_t p, xcb_gcontext_t gc)
{
	xcb_change_gc(jbxvt.X.xcb, gc, vm, (uint32_t[]){p});
	return p;
}

void reset_fg(void)
{
	jbxvt.X.color.current_fg = set_color(XCB_GC_FOREGROUND,
		jbxvt.X.color.fg, jbxvt.X.gc.tx);
}

void reset_bg(void)
{
	jbxvt.X.color.current_bg = set_color(XCB_GC_BACKGROUND,
		jbxvt.X.color.bg, jbxvt.X.gc.tx);
}

void reset_color(void)
{
	reset_fg();
	reset_bg();
}

void set_fg(const char * color)
{
	jbxvt.X.color.current_fg = set_color(GCForeground,
		get_pixel(color), jbxvt.X.gc.tx);
}

void set_bg(const char * color)
{
	jbxvt.X.color.current_bg = set_color(GCBackground,
		get_pixel(color), jbxvt.X.gc.tx);
}

