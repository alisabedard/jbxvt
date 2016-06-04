#include "color.h"

#include "jbxvt.h"

#ifdef USE_XCB
#include <stdlib.h>
#include <xcb/xproto.h>
#endif//USE_XCB

// returns pixel value for specified color
__attribute__((nonnull))
pixel_t get_pixel(const char * restrict color)
{
#ifdef USE_XCB
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
#else//!USE_XCB
	XColor c;

	XAllocNamedColor(jbxvt.X.dpy, jbxvt.X.color.map, color,
		&c, &(XColor){});

	return c.pixel;
#endif//USE_XCB
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((regparm(3)))
#endif//x86
static inline pixel_t set_color(const unsigned long vm,
	const pixel_t p,
#ifdef USE_XCB
	xcb_gcontext_t gc)
#else//!USE_XCB
	GC gc)
#endif//USE_XCB
{
#ifdef USE_XCB
	xcb_change_gc(jbxvt.X.xcb, XCBGC(gc),
		vm, (uint32_t[]){p});
#else//!USE_XCB
	XChangeGC(jbxvt.X.dpy, gc, vm, &(XGCValues){
		.foreground=p, .background=p});
#endif//USE_XCB
	return p;
}

void reset_fg(void)
{
#ifdef USE_XCB
	jbxvt.X.color.current_fg = set_color(XCB_GC_FOREGROUND,
		jbxvt.X.color.fg, jbxvt.X.gc.tx);
#else
	jbxvt.X.color.current_fg = set_color(GCForeground,
		jbxvt.X.color.fg, jbxvt.X.gc.tx);
#endif
}

void reset_bg(void)
{
#ifdef USE_XCB
	jbxvt.X.color.current_bg = set_color(XCB_GC_BACKGROUND,
		jbxvt.X.color.bg, jbxvt.X.gc.tx);
#else//!USE_XCB
	jbxvt.X.color.current_bg = set_color(GCBackground,
		jbxvt.X.color.bg, jbxvt.X.gc.tx);
#endif//USE_XCB
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

