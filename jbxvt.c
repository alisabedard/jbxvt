// Copyright 2016, Jeffrey E. Bedard
#include "command.h"
#include "config.h"
#include "cursor.h"
#include "display.h"
#include "tab.h"
#include "window.h"
#include "xevents.h"
#include "xvt.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static char ** parse_command_line(const int argc, char ** argv,
	struct JBXVTOptions * o)
{
	static const char * optstr = "B:b:C:c:D:d:eF:f:hI:R:S:sv";
	int8_t opt;
	while((opt=getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'B': // bold font
			o->font.bold = optarg;
			break;
		case 'b': // background color
			o->color.bg = optarg;
			break;
		case 'C': // columns
			o->size.cols = atoi(optarg);
			break;
		case 'c': // cursor style
			jbxvt_set_cursor_attr(atoi(optarg));
			break;
		case 'd': // screen number
			o->screen = atoi(optarg);
			break;
		case 'e': // exec
			return argv + optind;
		case 'F': // font
			o->font.normal = optarg;
			break;
		case 'f': // foreground color
			o->color.fg = optarg;
			break;
		case 'R': // rows
			o->size.rows = atoi(optarg);
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
static void set_default_options(struct JBXVTOptions * restrict o)
{
	o->font.normal = JBXVT_NORMAL_FONT;
	o->font.bold = JBXVT_BOLD_FONT;
	o->font.italic = JBXVT_ITALIC_FONT;
	o->color.bg = JBXVT_BACKGROUND_COLOR;
	o->color.fg = JBXVT_FOREGROUND_COLOR;
	o->size.cols = JBXVT_COLUMNS;
	o->size.rows = JBXVT_ROWS;
	o->screen = 0;
	o->show_scrollbar = false;
}
/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
int main(int argc, char ** argv)
{
	xcb_connection_t * xc;
	{ // com_argv scope
		char ** com_argv;
		{ // opt scope
			struct JBXVTOptions o;
			set_default_options(&o);
			// Override defaults
			com_argv = parse_command_line(argc, argv, &o);
			if (!com_argv)
				com_argv = (char*[2]){getenv("SHELL")};
			/* jbxvt_init_display must come
			   after parse_command_line */
			xc = jbxvt_init_display(argv[0], &o);
		}
		jbxvt_set_tab(-2, false); // Set up the tab stops
		jbxvt_map_window(xc);
		jb_check(setenv("TERM", JBXVT_ENV_TERM, true) != -1,
			"Could not set TERM environment variable");
		jbxvt_init_command_module(com_argv);
	}
	jbxvt_get_wm_del_win(xc); // initialize property
	for (;;) // app loop
		jbxvt_parse_token(xc);
	return 0;
}
