// Copyright 2016, Jeffrey E. Bedard
#include "command.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include "JBXVTOptions.h"
#include "color.h"
#include "config.h"
#include "cursor.h"
#include "display.h"
#include "font.h"
#include "libjb/JBDim.h"
#include "libjb/util.h"
#include "tab.h"
#include "window.h"
#include "xevents.h"
#include "xvt.h"
static char ** parse_command_line(const int argc, char ** argv,
	struct JBXVTOptions * o)
{
	static const char * optstr = "B:b:C:c:D:d:eF:f:hI:R:S:svx:y:";
	char opt;
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
		case 'x': // x position
			o->position.x = atoi(optarg);
			break;
		case 'y': // y position
			o->position.y = atoi(optarg);
			break;
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
static xcb_connection_t * handle_options(const int argc, char ** argv,
	char *** com_argv)
{
	// Set defaults:
	struct JBXVTOptions o = {
		.font.normal = JBXVT_NORMAL_FONT,
		.font.bold = JBXVT_BOLD_FONT,
		.font.italic = JBXVT_ITALIC_FONT,
		.color.bg = JBXVT_BACKGROUND_COLOR,
		.color.fg = JBXVT_FOREGROUND_COLOR,
		.size.cols = JBXVT_COLUMNS,
		.size.rows = JBXVT_ROWS,
	};
	// Override defaults:
	*com_argv = parse_command_line(argc, argv, &o);
	/* jbxvt_init_display must come
	   after parse_command_line */
	return jbxvt_init_display(argv[0], &o);
}
static xcb_connection_t * handle_command(const int argc, char ** argv)
{
	char ** com_argv;
	xcb_connection_t * xc = handle_options(argc, argv, &com_argv);
	jbxvt_set_tab(-2, false); // Set up the tab stops
	jbxvt_map_window(xc);
	jb_check(setenv("TERM", JBXVT_ENV_TERM, true) != -1,
		"Could not set TERM environment variable");
	if (!com_argv)
		com_argv = (char*[2]){getenv("SHELL")};
	jbxvt_init_command_module(com_argv);
	return xc;
}
/*  Run the command in a subprocess and return a file descriptor for
 *  the master end of the pseudo-teletype pair with the command
 *  talking to the slave.  */
int jbxvt_main(int argc, char ** argv)
{
	xcb_connection_t * xc = handle_command(argc, argv);
	(void)jbxvt_get_wm_del_win(xc); // initialize property
	while (jbxvt_parse_token(xc))
		;
	xcb_disconnect(xc);
	free(xc);
	return 0;
}
