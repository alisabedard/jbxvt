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


//  Error handling function, tidy up and then exit.
__attribute__((noreturn))
static int io_error_handler(Display * restrict dpy __attribute__((unused)))
{
	fprintf(stderr, "I/O error");
	quit(1);
}

__attribute__((noreturn))
static int error_handler(Display * restrict dpy __attribute__((unused)),
	XErrorEvent * restrict evp __attribute__((unused)))
{
	quit(1);
}

static void setup_sizehints(void)
{
	jbxvt.X.font = XLoadQueryFont(jbxvt.X.dpy, DEF_FONT);
	sizehints.width_inc = XTextWidth(jbxvt.X.font, "M", 1);
	sizehints.height_inc = jbxvt.X.font->ascent + jbxvt.X.font->descent;
	sizehints.width = 80 * sizehints.width_inc;
	sizehints.height = 24 * sizehints.height_inc;
	sizehints.min_width = sizehints.width_inc + sizehints.base_width;
	sizehints.min_height = sizehints.height_inc + sizehints.base_height;
	sizehints.flags |= USSize;
}

//  Open the window.
static void create_window(char * name)
{
	XTextProperty wname, iname;
	XClassHint class;
	XWMHints wmhints;
	Cursor cursor;

	jbxvt.X.win.main = XCreateSimpleWindow(jbxvt.X.dpy,
		DefaultRootWindow(jbxvt.X.dpy),
		sizehints.x,sizehints.y,sizehints.width,sizehints.height,
		0, jbxvt.X.color.fg,jbxvt.X.color.bg);

	if (XStringListToTextProperty(&name,1,&wname) == 0) {
		perror("cannot allocate window name");
		exit(1);
	}
	if (XStringListToTextProperty(&name,1,&iname) == 0) {
		perror("cannot allocate icon name");
		exit(1);
	}
	class.res_name = name;
	class.res_class = XVT_CLASS;
	wmhints.input = True;
	wmhints.initial_state = NormalState;
	wmhints.flags = InputHint | StateHint;
	XSetWMProperties(jbxvt.X.dpy, jbxvt.X.win.main, &wname,
		&iname, &name, 1, &sizehints, &wmhints, &class);
	XFree(iname.value);
	XFree(wname.value);

	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.main,MW_EVENTS);

	jbxvt.X.win.sb = XCreateSimpleWindow(jbxvt.X.dpy,
		jbxvt.X.win.main, -1, -1,
		SBAR_WIDTH - 1, sizehints.height, 1,
		jbxvt.X.color.fg, jbxvt.X.color.bg);

	cursor = XCreateFontCursor(jbxvt.X.dpy,XC_sb_v_double_arrow);
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.sb,cursor);

	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.sb,SB_EVENTS);

	jbxvt.X.win.vt = XCreateSimpleWindow(jbxvt.X.dpy, jbxvt.X.win.main,
		0, 0, sizehints.width, sizehints.height,0,
		jbxvt.X.color.fg, jbxvt.X.color.bg);

	// show scrollbar:
	XMoveWindow(jbxvt.X.dpy,jbxvt.X.win.vt,SBAR_WIDTH,0);
	XResizeWindow(jbxvt.X.dpy,jbxvt.X.win.vt,
		sizehints.width - SBAR_WIDTH,
		sizehints.height);

	cursor = XCreateFontCursor(jbxvt.X.dpy,XC_xterm);
	XDefineCursor(jbxvt.X.dpy,jbxvt.X.win.vt,cursor);
	XSelectInput(jbxvt.X.dpy,jbxvt.X.win.vt,VT_EVENTS);
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

void init_display(char * name)
{
	jbxvt.X.dpy = XOpenDisplay(NULL);
	if(!jbxvt.X.dpy) {
		perror("DISPLAY");
		exit(1);
	}
	uint8_t screen = DefaultScreen(jbxvt.X.dpy);
	jbxvt.X.color.map = DefaultColormap(jbxvt.X.dpy, screen);

	XSetErrorHandler(error_handler);
	XSetIOErrorHandler(io_error_handler);

	setup_sizehints();

	create_window(name);
	jbxvt.X.color.fg = jbxvt.X.color.cursor = WhitePixel(jbxvt.X.dpy, screen);
	jbxvt.X.color.bg = BlackPixel(jbxvt.X.dpy, screen);
	setup_gcs(jbxvt.X.dpy, jbxvt.X.win.main);
	scr_init();
	sbar_reset();
}

