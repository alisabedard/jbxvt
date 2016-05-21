#include "jbxvt.h"

#include "config.h"
#include "init_display.h"
#include "xsetup.h"
#include "xvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct JBXVT jbxvt;

// returns new command value if specified
static char ** parse_command_line(const int argc, char ** argv)
{
	const char * optstr = "cehvs";
	int8_t opt;
	while((opt=getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'e': // exec
			return argv + optind;
		case 's': // use scrollbar
			jbxvt.opt.show_scrollbar=true;
			break;
		case 'v': // version
			fprintf(stdout, "%s %s\n", argv[0], VERSION);
			exit(0);
		case 'h': // help
		default:
			fprintf(stdout, "%s -[%s]\n", argv[0], optstr);
			exit(0);
		}
	}
	return NULL;
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
int main(int argc, char ** argv)
{
	char ** com_argv = parse_command_line(argc, argv);
	init_display(argv[0]);
	map_window();
	char *shell_argv[2]; // here to not lose scope.
	if(!com_argv) {
		shell_argv[0] = getenv("SHELL");
		shell_argv[1] = NULL;
		com_argv = shell_argv;
	}
	setenv("TERM", TERM_ENV, true);
	init_command(com_argv);
	jbxvt_app_loop();

	return 0;
}
