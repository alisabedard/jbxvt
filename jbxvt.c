// Copyright 2016, Jeffrey E. Bedard
#include "jbxvt.h"
#include "command.h"
#include "display.h"
#include "tab.h"
#include "window.h"
#include "xvt.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
struct JBXVT jbxvt;
static char ** parse_command_line(const int argc, char ** argv,
	struct JBDim * size)
{
	static const char * optstr = "B:b:C:c:D:d:eF:f:hI:R:S:sv";
	int8_t opt;
	struct JBXVTOptionData * o = &jbxvt.opt;
	while((opt=getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'B': // bold font
			o->bold_font = optarg;
			break;
		case 'b': // background color
			o->bg = optarg;
			break;
		case 'C': // columns
			size->cols = atoi(optarg);
			break;
		case 'c': // cursor style
			jbxvt.opt.cursor_attr = atoi(optarg);
			break;
		case 'D': // DISPLAY
			o->display = optarg;
			break;
		case 'd': // screen number
			o->screen = atoi(optarg);
			break;
		case 'e': // exec
			return argv + optind;
		case 'F': // font
			o->font = optarg;
			break;
		case 'f': // foreground color
			o->fg = optarg;
			break;
		case 'R': // rows
			size->rows = atoi(optarg);
			break;
		case 's': // use scrollbar
			o->show_scrollbar=true;
			break;
		case 'v': // version
			goto version;
		case 'h': // help
		default:
			goto usage;
		}
	}
	return NULL;
version:
	printf("jbxvt %s\n", JBXVT_VERSION);
	exit(0);
usage:
	printf("%s -[%s]\n", argv[0], optstr);
	exit(0);
#ifdef OPENBSD
	return NULL;
#endif//OPENBSD
}
static void opt_init(void)
{
	// Set some defaults which may be overridden.
#define OPT(field, def) jbxvt.opt.field = JBXVT_##def
	OPT(fg, FG);
	OPT(bg, BG);
	OPT(font, FONT);
	OPT(bold_font, BOLD_FONT);
	OPT(italic_font, ITALIC_FONT);
#undef OPT
	// Default to a steady block cursor to conserve CPU
	jbxvt.opt.cursor_attr = 2;
	jbxvt.scr.sline.max = JBXVT_MAX_SCROLL;
}
/*  Perform any initialization on the screen data structures.
    Called just once at startup. */
static void jbxvt_init(void)
{
	// Initialise the array of lines that have scrolled off the top.
	struct JBXVTScreenData * s = &jbxvt.scr;
	static struct JBXVTScreen screens[2];
	s->current = s->s = screens;
	jbxvt_set_tab(-2, false);
}
// Set default values for private modes
static void mode_init(void)
{
	jbxvt.mode = (struct JBXVTPrivateModes) { .decanm = true,
		.decawm = false, .dectcem = true,
		.charset = {CHARSET_ASCII, CHARSET_ASCII}};
}
/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
int main(int argc, char ** argv)
{
	opt_init();
	xcb_connection_t * xc;
	struct JBDim size = {.cols = JBXVT_COLUMNS, .rows = JBXVT_ROWS};
	char ** com_argv = parse_command_line(argc, argv, &size);
	if (!com_argv)
		com_argv = (char*[2]){getenv("SHELL")};
	// jbxvt_init_display must come after parse_command_line
	xc = jbxvt_init_display(argv[0], size);
	mode_init();
	jbxvt_init();
	jbxvt_map_window(xc);
	jb_check(setenv("TERM", JBXVT_ENV_TERM, true) != -1,
		"Could not set TERM environment variable");
	jbxvt_init_command_module(xc, com_argv);
	for (;;) // app loop
		jbxvt_parse_token(xc);
	return 0;
}
