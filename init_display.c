#include "init_display.h"


#include "jbxvt.h"
#include "sbar.h"
#include "screen.h"
#include "ttyinit.h"
#include "xsetup.h"
#include "xvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>


static char * jbxvt_name;

static bool console_flag;

//  Return if this window should be a console.
inline bool is_console(void)
{
	return(console_flag);
}

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


/*  Take a pass through the arguments extracting any that do not correspond
 *  to X resources.  Recognised arguments are removed from the list and
 *  the new value of argc is returned.
 */
static uint8_t extract_nonX_args(int argc, char ** argv)
{
	uint8_t i, j;

	jbxvt_name = argv[0];
	for (j = i = 1; i < argc; i++) {
		if (strcmp(argv[i],"-name") == 0) {
			if (argv[++i])
				jbxvt_name = argv[i];
			else fprintf(stderr, "missing -name argument");
		} else if (strcmp(argv[i],"-C") == 0
			|| strcmp(argv[i],"-console") == 0) {
			console_flag = 1;
		} else
			argv[j++] = argv[i];
	}
	argv[j] = NULL;
	return(j);
}

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
static void create_window(int argc, char ** argv)
{
	XTextProperty wname, iname;
	XClassHint class;
	XWMHints wmhints;
	Cursor cursor;

	jbxvt.X.win.main = XCreateSimpleWindow(jbxvt.X.dpy,
		DefaultRootWindow(jbxvt.X.dpy),
		sizehints.x,sizehints.y,sizehints.width,sizehints.height,
		0, jbxvt.X.color.fg,jbxvt.X.color.bg);

	if (XStringListToTextProperty(&argv[0],1,&wname) == 0) {
		perror("cannot allocate window name");
		exit(1);
	}
	if (XStringListToTextProperty(&argv[0],1,&iname) == 0) {
		perror("cannot allocate icon name");
		exit(1);
	}
	class.res_name = argv[0];
	class.res_class = XVT_CLASS;
	wmhints.input = True;
	wmhints.initial_state = NormalState;
	wmhints.flags = InputHint | StateHint;
	XSetWMProperties(jbxvt.X.dpy,jbxvt.X.win.main,&wname,&iname,argv,argc,
		&sizehints,&wmhints,&class);
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

void init_display(int argc, char ** argv, int iargc, char ** iargv)
{
	argc = extract_nonX_args(argc,argv);
	jbxvt.X.dpy = XOpenDisplay(NULL);
	if(!jbxvt.X.dpy) {
		fprintf(stderr, "%s",
			"Cannot open display, make sure environment variable"
			" DISPLAY is set");
		exit(1);
	}
	uint8_t screen = DefaultScreen(jbxvt.X.dpy);
	jbxvt.X.color.map = DefaultColormap(jbxvt.X.dpy, screen);

	XSetErrorHandler(error_handler);
	XSetIOErrorHandler(io_error_handler);

	setup_sizehints();

	create_window(iargc, iargv);
	jbxvt.X.color.fg = jbxvt.X.color.cursor = WhitePixel(jbxvt.X.dpy, screen);
	jbxvt.X.color.bg = BlackPixel(jbxvt.X.dpy, screen);
	setup_gcs(jbxvt.X.dpy, jbxvt.X.win.main);
	scr_init(DEF_SAVED_LINES);
	sbar_init();
}

