#include "color.h"

#include "jbxvt.h"

// returns pixel value for specified color
pixel_t get_pixel(const char * restrict color)
{
	XColor c;

	XAllocNamedColor(jbxvt.X.dpy, jbxvt.X.color.map, color,
		&c, &(XColor){});

	return c.pixel;
}

static void set_color(const char * restrict color, GC gc,
	const unsigned long vm)
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
	set_color(color, jbxvt.X.gc.tx, GCForeground);
}

void set_bg(const char * color)
{
	set_color(color, jbxvt.X.gc.tx, GCBackground);
}

