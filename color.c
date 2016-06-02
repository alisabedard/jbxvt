#include "color.h"

#include "jbxvt.h"

// returns pixel value for specified color
__attribute__((nonnull))
pixel_t get_pixel(const char * restrict color)
{
	XColor c;

	XAllocNamedColor(jbxvt.X.dpy, jbxvt.X.color.map, color,
		&c, &(XColor){});

	return c.pixel;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((regparm(1), nonnull))
#else//!x86
	__attribute__((nonnull))
#endif//x86
static inline void set_color(const unsigned long vm,
	const char * restrict color, GC gc)
{
	const pixel_t p = get_pixel(color);

	XChangeGC(jbxvt.X.dpy, gc, vm, &(XGCValues){
		.foreground=p, .background=p});
}

void reset_fg(void)
{
	XGCValues v = { .foreground = jbxvt.X.color.fg };
	XChangeGC(jbxvt.X.dpy, jbxvt.X.gc.tx, GCForeground, &v);
}

void reset_bg(void)
{
	XGCValues v = { .background = jbxvt.X.color.bg	};
	XChangeGC(jbxvt.X.dpy, jbxvt.X.gc.tx, GCBackground, &v);
}

void reset_color(void)
{
	reset_fg();
	reset_bg();
}

void set_fg(const char * color)
{
	set_color(GCForeground, color, jbxvt.X.gc.tx);
}

void set_bg(const char * color)
{
	set_color(GCBackground, color, jbxvt.X.gc.tx);
}

