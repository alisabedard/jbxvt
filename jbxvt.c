// Copyright 2016, Jeffrey E. Bedard

#include "jbxvt.h"

#include "config.h"
#include "init_display.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "screen.h"
#include "xsetup.h"
#include "xvt.h"

#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

JBXVT jbxvt;

static char ** parse_command_line(const int argc, char ** argv)
{
	static const char * optstr = "B:b:C:c:D:d:eF:f:ehvR:S:s";
	int8_t opt;
	while((opt=getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'B': // bold font
			jbxvt.opt.bold_font = optarg;
			break;
		case 'b': // background color
			jbxvt.opt.bg = optarg;
			break;
		case 'D': // DISPLAY
			jbxvt.opt.display = optarg;
			break;
		case 'd': // screen number
			jbxvt.opt.screen = atoi(optarg);
			break;
		case 'C': // columns
			jbxvt.opt.size.cols = atoi(optarg);
			break;
		case 'e': // exec
			return argv + optind;
		case 'F': // font
			jbxvt.opt.font = optarg;
			break;
		case 'f': // foreground color
			jbxvt.opt.fg = optarg;
			break;
		case 'R': // rows
			jbxvt.opt.size.rows = atoi(optarg);
			break;
		case 'S': // scroll history
			jbxvt.scr.sline.max = atoi(optarg);
			break;
		case 's': // use scrollbar
			jbxvt.opt.show_scrollbar=true;
			break;
		case 'v': // version
			puts("jbxvt" VERSION);
			exit(0);
		case 'h': // help
		default:
			printf("%s -[%s]\n", argv[0], optstr);
			exit(0);
		}
	}
	return NULL;
}

static void set_defaults(void)
{
	// Set some defaults which may be overridden.
	jbxvt.opt.fg = JBXVT_FG;
	jbxvt.opt.bg = JBXVT_BG;
	jbxvt.opt.font = DEF_FONT;
	jbxvt.opt.bold_font = BOLD_FONT;
	jbxvt.opt.size.width = JBXVT_COLUMNS;
	jbxvt.opt.size.height = JBXVT_ROWS;
}

/*  Perform any initialization on the screen data structures.
    Called just once at startup. */
static void scr_init(void)
{
	// Initialise the array of lines that have scrolled off the top.
	jbxvt.scr.sline.max = MAX_SCROLL;
	jbxvt.scr.sline.data = GC_MALLOC(jbxvt.scr.sline.max * sizeof(void*));
	jbxvt.mode.decawm = true;
	jbxvt.mode.dectcem = true;
	jbxvt.mode.charset[0] = CHARSET_ASCII;
	jbxvt.mode.charset[1] = CHARSET_ASCII;
	jbxvt.scr.current = &jbxvt.scr.s[0];
	scr_reset();
	scr_set_tab(-2, false);
}


/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
int main(int argc, char ** argv)
{
	GC_INIT();
	set_defaults();
	char ** com_argv = parse_command_line(argc, argv);
	init_display(argv[0]);
	jbxvt.opt.cursor_attr = 2; // steady block
	scr_init();
	map_window();
	char *shell_argv[2]; // here to not lose scope.
	if(!com_argv) {
		shell_argv[0] = getenv("SHELL");
		shell_argv[1] = NULL;
		com_argv = shell_argv;
	}
	jb_check(setenv("TERM", TERM_ENV, true) != -1,
		"Could not set TERM environment variable");
	init_command(com_argv);
	jbxvt_app_loop();

	return 0;
}

