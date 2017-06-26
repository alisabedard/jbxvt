// Copyright 2017, Jeffrey E. Bedard
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "JBXVTOptions.h"
#include "config.h"
#include "cursor.h"
#include "display.h"
#include "tab.h"
#include "window.h"
#include "xvt.h"
static char ** parse_command_line(const int argc, char ** argv,
	struct JBXVTOptions * o)
{
	static const char * optstr = "B:b:C:c:D:d:eF:f:hH:I:R:S:svW:x:y:";
	int opt;
	int16_t optval;
#define GET_OPTVAL() optval = (int16_t)atoi(optarg);
	while((opt=getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'B': // bold font
			o->bold_font = optarg;
			break;
		case 'b': // background color
			o->background_color = optarg;
			break;
		case 'W': // width
		case 'C': // columns
			GET_OPTVAL();
			o->size.cols = optval;
			break;
		case 'c': // cursor style
			GET_OPTVAL();
			jbxvt_set_cursor_attr((uint8_t)optval);
			break;
		case 'd': // screen number
			GET_OPTVAL();
			o->screen = (uint8_t)optval;
			break;
		case 'e': // exec
			return argv + optind;
		case 'F': // font
			o->normal_font = optarg;
			break;
		case 'f': // foreground color
			o->foreground_color = optarg;
			break;
		case 'H': // height
		case 'R': // rows
			GET_OPTVAL();
			o->size.rows = optval;
			break;
		case 's': // use scrollbar
			o->show_scrollbar = true;
			break;
		case 'v': // version
			printf("jbxvt %s\n", JBXVT_VERSION);
			exit(0);
		case 'x': // x position
			GET_OPTVAL();
			o->position.x = optval;
			break;
		case 'y': // y position
			GET_OPTVAL();
			o->position.y = optval;
			break;
		case 'h': // help
		default:
			printf("%s -[%s]\n", argv[0], optstr);
			exit(0);
		}
	}
	return NULL;
}
static xcb_connection_t * handle_options(const int argc, char ** argv,
	char *** com_argv)
{
	// Set defaults:
	struct JBXVTOptions o = {
		.normal_font = JBXVT_NORMAL_FONT,
		.bold_font = JBXVT_BOLD_FONT,
		.italic_font = JBXVT_ITALIC_FONT,
		.background_color = JBXVT_BACKGROUND_COLOR,
		.foreground_color = JBXVT_FOREGROUND_COLOR,
		.size.cols = JBXVT_COLUMNS,
		.size.rows = JBXVT_ROWS,
	};
	// Override defaults:
	*com_argv = parse_command_line(argc, argv, &o);
	/* jbxvt_init_display must come
	   after parse_command_line */
	return jbxvt_init_display(argv[0], &o);
}
static xcb_connection_t * init(const int argc, char ** argv)
{
	char ** com_argv;
	xcb_connection_t * xc = handle_options(argc, argv, &com_argv);
	if (setenv("TERM", JBXVT_ENV_TERM, true) < 0)
		abort();
	if (!com_argv)
		com_argv = (char*[2]){getenv(JBXVT_ENV_SHELL)};
	jbxvt_init_command_module(com_argv);
	return xc;
}
/*  Run the command in a subprocess and return a file descriptor for
 *  the master end of the pseudo-teletype pair with the command
 *  talking to the slave.  */
int main(int argc, char ** argv)
{
	xcb_connection_t * xc = init(argc, argv);
	jbxvt_set_tab(JBXVT_SET_TAB_RESET, false); // Set up the tab stops
	jbxvt_map_window(xc);
	while (jbxvt_parse_token(xc))
		;
	xcb_disconnect(xc);
	free(xc);
	return 0;
}
