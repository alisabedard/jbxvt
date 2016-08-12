// Copyright 2016, Jeffrey E. Bedard

#include "jbxvt.h"

#include "config.h"
#include "init_display.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "screen.h"
#include "xsetup.h"
#include "xvt.h"

#include <errno.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct JBXVT jbxvt;

static char ** parse_command_line(const int argc, char ** argv)
{
	static const char * optstr = "B:b:C:c:D:d:eF:f:ehvR:S:s";
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
		case 'D': // DISPLAY
			o->display = optarg;
			break;
		case 'd': // screen number
			o->screen = atoi(optarg);
			break;
		case 'C': // columns
			o->size.cols = atoi(optarg);
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
			o->size.rows = atoi(optarg);
			break;
		case 'S': // scroll history
			jbxvt.scr.sline.max = atoi(optarg);
			break;
		case 's': // use scrollbar
			o->show_scrollbar=true;
			break;
		case 'v': // version
			puts("jbxvt" JBXVT_VERSION);
			exit(0);
		case 'h': // help
		default:
			printf("%s -[%s]\n", argv[0], optstr);
			exit(0);
		}
	}
	return NULL;
}

static void opt_init(void)
{
	// Set some defaults which may be overridden.
#define OPT(field, def) jbxvt.opt.field = JBXVT_##def
	OPT(fg, FG);
	OPT(bg, BG);
	OPT(font, FONT);
	OPT(bold_font, BOLD_FONT);
	OPT(size.width, COLUMNS);
	OPT(size.height, ROWS);
}

/*  Perform any initialization on the screen data structures.
    Called just once at startup. */
static void scr_init(void)
{
	// Initialise the array of lines that have scrolled off the top.
	struct JBXVTScreenData * s = &jbxvt.scr;
	s->sline.max = MAX_SCROLL;
	s->sline.data = GC_MALLOC(s->sline.max * sizeof(void*));
	s->s = GC_MALLOC(sizeof(VTScreen)<<1);
	s->current = &s->s[0];
	scr_set_tab(-2, false);
}

// Set default values for private modes
static void mode_init(void)
{
	struct JBXVTPrivateModes * m = &jbxvt.mode;
	m->decanm = m->decawm = m->dectcem = true;
	m->charset[0] = m->charset[1] = CHARSET_ASCII;
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
int main(int argc, char ** argv)
{
	errno = 0;
	GC_INIT();
	opt_init();
	char ** com_argv = parse_command_line(argc, argv);
	if (!com_argv)
		com_argv = (char*[2]){getenv("SHELL")};
	// init_display must come after parse_command_line
	init_display(argv[0]);
	jbxvt.opt.cursor_attr = 2; // steady block
	mode_init();
	scr_init();
	map_window();
	jb_check(setenv("TERM", TERM_ENV, true) != -1,
		"Could not set TERM environment variable");
	init_command(com_argv);
	jbxvt_app_loop();

	return 0;
}

