/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "init_display.h"

#include "config.h"
#include "jbxvt.h"
#include "sbar.h"
#include "screen.h"
#include "ttyinit.h"
#include "xsetup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

#define XVT_CLASS	"JBXvt"

#define MW_EVENTS	(	KeyPressMask |\
				FocusChangeMask |\
				StructureNotifyMask \
			)

#define VT_EVENTS	(	ExposureMask |\
				EnterWindowMask|\
				LeaveWindowMask |\
				ButtonPressMask |\
				ButtonReleaseMask |\
				Button1MotionMask \
			)


#define SB_EVENTS	(	ExposureMask |\
				EnterWindowMask|\
				LeaveWindowMask |\
				Button2MotionMask |\
				ButtonReleaseMask |\
				ButtonPressMask \
			)

static void setup_font(void)
{
	jbxvt.X.font = XLoadQueryFont(jbxvt.X.dpy,
		jbxvt.opt.font?jbxvt.opt.font:DEF_FONT); // if specified
	if(!jbxvt.X.font) // fallback
		  jbxvt.X.font = XLoadQueryFont(jbxvt.X.dpy, FIXED_FONT);
	if(!jbxvt.X.font) // fail
		  quit(1, WARN_RES RES_FNT);
}

// free the returned value
static XSizeHints * get_sizehints(void)
{
	XSizeHints * s = malloc(sizeof(XSizeHints));
	*s = (XSizeHints) {
		.flags = USSize | PMinSize | PResizeInc | PBaseSize,
		.width = 80, .height = 24,
		.base_width = MARGIN<<1,
		.base_height = MARGIN<<1,
		.width_inc = XTextWidth(jbxvt.X.font, "M", 1),
		.height_inc = jbxvt.X.font->ascent
			+ jbxvt.X.font->descent
	};
	s->width *= s->width_inc;
	s->height *= s->height_inc;
	s->min_width = s->width_inc + s->base_width;
	s->min_height = s->height_inc + s->base_height;

	return s;
}

static void create_main_window(XSizeHints * restrict sh)
{
	jbxvt.X.win.main = XCreateSimpleWindow(jbxvt.X.dpy,
		DefaultRootWindow(jbxvt.X.dpy),
		sh->x, sh->y, sh->width, sh->height,
		0, jbxvt.X.color.fg,jbxvt.X.color.bg);
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.main, MW_EVENTS);
}

static void create_sb_window(const uint16_t height)
{
	jbxvt.X.win.sb = XCreateSimpleWindow(jbxvt.X.dpy,
		jbxvt.X.win.main, -1, -1,
		SBAR_WIDTH - 1, height, 1,
		jbxvt.X.color.fg, jbxvt.X.color.bg);
	XSelectInput(jbxvt.X.dpy, jbxvt.X.win.sb, SB_EVENTS);
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.sb,
		XCreateFontCursor(jbxvt.X.dpy, XC_sb_v_double_arrow));
}

static void create_vt_window(XSizeHints * restrict sh)
{
	jbxvt.X.win.vt = XCreateSimpleWindow(jbxvt.X.dpy,
		jbxvt.X.win.main, 0, 0, sh->width, sh->height, 0,
		jbxvt.X.color.fg, jbxvt.X.color.bg);
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.vt,
		XCreateFontCursor(jbxvt.X.dpy, XC_xterm));
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.vt, VT_EVENTS);
}

//  Open the window.
static void create_window(uint8_t * restrict name)
{
	XSizeHints * sh = get_sizehints();
	create_main_window(sh);
	change_name(name, true, true);
	create_sb_window(sh->height);
	create_vt_window(sh);
	free(sh);
	jbxvt.opt.show_scrollbar ^= true;
	switch_scrollbar();
}

static void setup_gcs(void)
{
	XGCValues gcv = { .foreground = jbxvt.X.color.fg,
		.background = jbxvt.X.color.bg,
		.function = GXinvert, .font = jbxvt.X.font->fid,
		.graphics_exposures = False };
	jbxvt.X.gc.tx = XCreateGC(jbxvt.X.dpy, jbxvt.X.win.main,
		GCForeground|GCBackground|GCFont, &gcv);
	jbxvt.X.gc.ne = XCreateGC(jbxvt.X.dpy, jbxvt.X.win.main,
		GCForeground|GCBackground|GCGraphicsExposures, &gcv);
	jbxvt.X.gc.sb = XCreateGC(jbxvt.X.dpy, jbxvt.X.win.main,
		GCForeground|GCBackground|GCFont, &gcv);
	gcv.plane_mask = jbxvt.X.color.fg ^ jbxvt.X.color.bg;
	jbxvt.X.gc.hl = XCreateGC(jbxvt.X.dpy, jbxvt.X.win.main,
	       GCFunction|GCPlaneMask, &gcv);
	gcv.plane_mask = jbxvt.X.color.cursor ^ jbxvt.X.color.bg;
	jbxvt.X.gc.cu = XCreateGC(jbxvt.X.dpy, jbxvt.X.win.main,
		GCFunction|GCPlaneMask, &gcv);
}

static void init_jbxvt_colors(void)
{
	jbxvt.X.color.map = DefaultColormap(jbxvt.X.dpy,
		jbxvt.X.screen);
	const pixel_t p[] = {
		BlackPixel(jbxvt.X.dpy, jbxvt.X.screen),
		WhitePixel(jbxvt.X.dpy, jbxvt.X.screen)
	};
	jbxvt.X.color.fg = jbxvt.opt.fg?get_pixel(jbxvt.opt.fg):p[1];
	jbxvt.X.color.cursor = jbxvt.opt.cu?get_pixel(jbxvt.opt.cu):p[1];
	jbxvt.X.color.bg = jbxvt.opt.bg?get_pixel(jbxvt.opt.bg):p[0];
}

void init_display(char * name)
{
	jbxvt.X.dpy = XOpenDisplay(NULL);
#ifdef USE_XCB
	jbxvt.X.xcb = XGetXCBConnection(jbxvt.X.dpy);
#endif//USE_XCB
	if(!jbxvt.X.dpy)
		  quit(1, WARN_RES RES_DPY);

	jbxvt.X.screen = DefaultScreen(jbxvt.X.dpy);
	init_jbxvt_colors();
	setup_font();
	create_window((uint8_t *)name);
	setup_gcs();

	scr_init();
	sbar_reset();
}

