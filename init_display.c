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
}

static void setup_sizehints(void)
{
	sizehints.width_inc = XTextWidth(jbxvt.X.font, "M", 1);
	sizehints.height_inc = jbxvt.X.font->ascent + jbxvt.X.font->descent;
	sizehints.width = 80 * sizehints.width_inc;
	sizehints.height = 24 * sizehints.height_inc;
	sizehints.min_width = sizehints.width_inc + sizehints.base_width;
	sizehints.min_height = sizehints.height_inc + sizehints.base_height;
	sizehints.flags |= USSize;
}

static void setup_properties(char * name)
{
	XClassHint class = { .res_name = name, .res_class = XVT_CLASS };
	XWMHints wmhints = { .input = true, .initial_state = NormalState,
		.flags = InputHint | StateHint};
	XTextProperty winame;
	XStringListToTextProperty(&name, 1, &winame);
	XSetWMProperties(jbxvt.X.dpy, jbxvt.X.win.main, &winame,
		&winame, &name, 1, &sizehints, &wmhints, &class);
	XFree(winame.value);
}

static void create_main_window(void)
{
	jbxvt.X.win.main = XCreateSimpleWindow(jbxvt.X.dpy,
		DefaultRootWindow(jbxvt.X.dpy),
		sizehints.x,sizehints.y,sizehints.width,sizehints.height,
		0, jbxvt.X.color.fg,jbxvt.X.color.bg);
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.main,MW_EVENTS);
}

static void create_sb_window(void)
{
	jbxvt.X.win.sb = XCreateSimpleWindow(jbxvt.X.dpy,
		jbxvt.X.win.main, -1, -1,
		SBAR_WIDTH - 1, sizehints.height, 1,
		jbxvt.X.color.fg, jbxvt.X.color.bg);
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.sb,SB_EVENTS);
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.sb,
		XCreateFontCursor(jbxvt.X.dpy, XC_sb_v_double_arrow));
}

static void create_vt_window(void)
{
	jbxvt.X.win.vt = XCreateSimpleWindow(jbxvt.X.dpy, jbxvt.X.win.main,
		0, 0, sizehints.width, sizehints.height, 0,
		jbxvt.X.color.fg, jbxvt.X.color.bg);
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.vt,
		XCreateFontCursor(jbxvt.X.dpy, XC_xterm));
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.vt,VT_EVENTS);
}

//  Open the window.
static void create_window(char * name)
{
	setup_sizehints();
	create_main_window();
	setup_properties(name);
	create_sb_window();
	create_vt_window();

	if(jbxvt.opt.show_scrollbar) { // show scrollbar:
		XMoveWindow(jbxvt.X.dpy,jbxvt.X.win.vt,SBAR_WIDTH,0);
		XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.vt,
			sizehints.width - SBAR_WIDTH,
			sizehints.height);
	}

}

static void setup_gcs(Display * d, Window w)
{
	XGCValues gcv = { .foreground = jbxvt.X.color.fg,
		.background = jbxvt.X.color.bg,
		.function = GXinvert, .font = jbxvt.X.font->fid,
		.graphics_exposures = False };
	jbxvt.X.gc.tx = XCreateGC(d, w, GCForeground|GCBackground|GCFont, &gcv);
	jbxvt.X.gc.ne = XCreateGC(d, w,
		GCForeground|GCBackground|GCGraphicsExposures, &gcv);
	jbxvt.X.gc.sb = XCreateGC(d, w, GCForeground|GCBackground|GCFont, &gcv);
	gcv.plane_mask = jbxvt.X.color.fg ^ jbxvt.X.color.bg;
	jbxvt.X.gc.hl = XCreateGC(d, w, GCFunction|GCPlaneMask, &gcv);
	gcv.plane_mask = jbxvt.X.color.cursor ^ jbxvt.X.color.bg;
	jbxvt.X.gc.cu = XCreateGC(d, w, GCFunction|GCPlaneMask, &gcv);
}

static void init_jbxvt_colors(void)
{
	jbxvt.X.color.map = DefaultColormap(jbxvt.X.dpy, jbxvt.X.screen);
	const pixel_t wp = WhitePixel(jbxvt.X.dpy, jbxvt.X.screen);
	jbxvt.X.color.fg = jbxvt.opt.fg?get_pixel(jbxvt.opt.fg):wp;
	jbxvt.X.color.cursor = jbxvt.opt.cu?get_pixel(jbxvt.opt.cu):wp;
	const pixel_t bp = BlackPixel(jbxvt.X.dpy, jbxvt.X.screen);
	jbxvt.X.color.bg = jbxvt.opt.bg?get_pixel(jbxvt.opt.bg):bp;
}

void init_display(char * name)
{
	jbxvt.X.dpy = XOpenDisplay(NULL);
	if(!jbxvt.X.dpy)
		  quit(1, QUIT_DISPLAY);

	jbxvt.X.screen = DefaultScreen(jbxvt.X.dpy);
	init_jbxvt_colors();
	setup_font();
	create_window(name);
	setup_gcs(jbxvt.X.dpy, jbxvt.X.win.main);

	scr_init();
	sbar_reset();
}

