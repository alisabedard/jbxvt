// Copyright 2017, Jeffrey E. Bedard <jefbed@gmail.com>
extern "C" {
#include "jbxvt.h"
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include "JBXVTOptions.h"
#include "config.h"
#include "cursor.h"
#include "display.h"
#include "window.h"
#include "xvt.h"
}
#include <string>
#include <cstdio>
using namespace std;
class Argv {
	public:
		Argv();
		char ** parse(int argc, char ** argv);
		struct JBXVTOptions * get(void)
		{
			return &opt;
		}
	private:
		struct JBXVTOptions opt;
		string normal, bold, italic, bg, fg;
		static char ** parse_command_line(const int argc,
			char ** argv, struct JBXVTOptions * o);
};
Argv::Argv()
	: normal(JBXVT_NORMAL_FONT), bold(JBXVT_BOLD_FONT),
	italic(JBXVT_ITALIC_FONT), bg(JBXVT_BACKGROUND_COLOR),
	fg(JBXVT_FOREGROUND_COLOR), opt((struct JBXVTOptions)
		{.size.rows=JBXVT_ROWS, .size.columns=JBXVT_COLUMNS})
{
	opt.font.normal = (char *)normal.c_str();
	opt.font.bold = (char *)bold.c_str();
	opt.font.italic = (char *)italic.c_str();
	opt.color.bg = (char *)bg.c_str();
	opt.color.fg = (char *)fg.c_str();
}
char ** Argv::parse(const int argc, char ** argv)
{
	return parse_command_line(argc, argv, &opt);
}
char ** Argv::parse_command_line(const int argc, char ** argv,
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
class Main {
	public:
		Main(int argc, char ** argv)
		{
			Argv p;
			char ** com_argv = p.parse(argc, argv);
			xc = jbxvt_init_display(argv[0], p.get());
			setenv("TERM", JBXVT_ENV_TERM, true);
			if (!com_argv)
				com_argv = (char *[2]){getenv("SHELL")};
			jbxvt_init_command_module(com_argv);
//			jbxvt_main(argc, argv);
		}
		~Main()
		{
			xcb_disconnect(xc);
			free(xc);
		}
		void run(void)
		{
			jbxvt_set_tab(-2, false);
			jbxvt_map_window(xc);
			while (jbxvt_parse_token(xc))
				;
		}
	private:
		xcb_connection_t * xc;
};
int main(int argc, char ** argv)
{
	Main jbxvt(argc, argv);
	return 0;
}
