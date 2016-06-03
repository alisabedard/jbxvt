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

static void create_main_window(XSizeHints * restrict sh, const uint32_t root)
{
#ifdef USE_XCB
	jbxvt.X.win.main = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_window(jbxvt.X.xcb, XCB_COPY_FROM_PARENT,
		jbxvt.X.win.main, root, sh->x, sh->y,
		sh->width, sh->height, 0, XCB_WINDOW_CLASS_COPY_FROM_PARENT,
		XCB_COPY_FROM_PARENT, XCB_CW_COLORMAP | XCB_CW_EVENT_MASK,
		(uint32_t[]){MW_EVENTS, jbxvt.X.color.map});
#else//!USE_XCB
	jbxvt.X.win.main = XCreateSimpleWindow(jbxvt.X.dpy, root,
		sh->x, sh->y, sh->width, sh->height,
		0, jbxvt.X.color.fg,jbxvt.X.color.bg);
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.main, MW_EVENTS);
#endif//USE_XCB
}

static void create_sb_window(const uint16_t height)
{
#ifdef USE_XCB
	jbxvt.X.win.sb = xcb_generate_id(jbxvt.X.xcb);
	xcb_create_window(jbxvt.X.xcb, XCB_COPY_FROM_PARENT,
		jbxvt.X.win.sb, jbxvt.X.win.main, -1, -1,
		SBAR_WIDTH - 1, height, 1,
		XCB_WINDOW_CLASS_COPY_FROM_PARENT,
		XCB_COPY_FROM_PARENT, XCB_CW_BACK_PIXEL | XCB_CW_COLORMAP
		| XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK,
		(uint32_t[]){jbxvt.X.color.bg, jbxvt.X.color.fg,
		SB_EVENTS, jbxvt.X.color.map});
#else//!USE_XCB
	jbxvt.X.win.sb = XCreateSimpleWindow(jbxvt.X.dpy,
		jbxvt.X.win.main, -1, -1,
		SBAR_WIDTH - 1, height, 1,
		jbxvt.X.color.fg, jbxvt.X.color.bg);
	XSelectInput(jbxvt.X.dpy, jbxvt.X.win.sb, SB_EVENTS);
#endif//USE_XCB
	// FIXME: do in xcb
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.sb,
		XCreateFontCursor(jbxvt.X.dpy, XC_sb_v_double_arrow));
}

static void create_vt_window(XSizeHints * restrict sh)
{
#ifdef USE_XCB
	jbxvt.X.win.vt = xcb_generate_id(jbxvt.X.xcb);

	xcb_create_window(jbxvt.X.xcb, XCB_COPY_FROM_PARENT,
		jbxvt.X.win.vt, jbxvt.X.win.main,
		0, 0, sh->width, sh->height, 0,
		XCB_WINDOW_CLASS_COPY_FROM_PARENT,
		XCB_COPY_FROM_PARENT, XCB_CW_BACK_PIXEL|XCB_CW_COLORMAP
		|XCB_CW_EVENT_MASK,
		(uint32_t[]){jbxvt.X.color.bg, VT_EVENTS, jbxvt.X.color.map});
#else//!USE_XCB
	jbxvt.X.win.vt = XCreateSimpleWindow(jbxvt.X.dpy,
		jbxvt.X.win.main, 0, 0, sh->width, sh->height, 0,
		jbxvt.X.color.fg, jbxvt.X.color.bg);
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.vt, VT_EVENTS);
#endif//USE_XCB
	// FIXME: do in xcb
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.vt,
		XCreateFontCursor(jbxvt.X.dpy, XC_xterm));
}

//  Open the window.
static void create_window(uint8_t * restrict name, const Window root)
{
	XSizeHints * sh = get_sizehints();
	create_main_window(sh, root);
	change_name(name, true);
	change_name(name, false);
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
#ifdef USE_XCB
	jbxvt.X.color.map = jbxvt.X.screen->default_colormap;
	const pixel_t p[] = {
		jbxvt.X.screen->black_pixel,
		jbxvt.X.screen->white_pixel
	};
#else//!USE_XCB
	jbxvt.X.color.map = DefaultColormap(jbxvt.X.dpy, screen);
	const uint8_t screen = DefaultScreen(jbxvt.X.dpy);
	const pixel_t p[] = {
		BlackPixel(jbxvt.X.dpy, screen),
		WhitePixel(jbxvt.X.dpy, screen)
	};
#endif//USE_XCB
	jbxvt.X.color.fg = jbxvt.opt.fg?get_pixel(jbxvt.opt.fg):p[1];
	jbxvt.X.color.cursor = jbxvt.opt.cu?get_pixel(jbxvt.opt.cu):p[1];
	jbxvt.X.color.bg = jbxvt.opt.bg?get_pixel(jbxvt.opt.bg):p[0];
	jbxvt.X.color.current_fg = jbxvt.X.color.fg;
	jbxvt.X.color.current_bg = jbxvt.X.color.bg;
}

void init_display(char * name)
{
	jbxvt.X.dpy = XOpenDisplay(NULL);
	if(!jbxvt.X.dpy)
		  quit(1, WARN_RES RES_DPY);
#ifdef USE_XCB
	jbxvt.X.xcb = XGetXCBConnection(jbxvt.X.dpy);
	jbxvt.X.screen = xcb_setup_roots_iterator(
		xcb_get_setup(jbxvt.X.xcb)).data;
	const Window root = jbxvt.X.screen->root;
#else//!USE_XCB
	const Window root = DefaultRootWindow(jbxvt.X.dpy);
#endif//USE_XCB
	init_jbxvt_colors();
	setup_font();
	create_window((uint8_t *)name, root);
	setup_gcs();

	scr_init();
	sbar_reset();
}

