#include "jbxvt.h"

#include "config.h"
#include "init_display.h"
#include "ttyinit.h"
#include "xsetup.h"
#include "xvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

struct JBXVT jbxvt;

// Print string to stderr
void jbputs(const char * string)
{
        size_t s = 0;
        while(string[++s]);
#ifdef SYS_write
	syscall(SYS_write, STDERR_FILENO, string, s);
#else//!SYS_write
	write(STDERR_FILENO, string, s);
#endif//SYS_write
}


static char ** parse_command_line(const int argc, char ** argv)
{
	static const char * optstr = "b:c:eF:f:ehvS:s";
	int8_t opt;
	while((opt=getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'b': // background color
			jbxvt.opt.bg = optarg;
			break;
		case 'c': // cursor color
			jbxvt.opt.cu = optarg;
			break;
		case 'e': // exec
			return argv + optind;
		case 'F': // font
			jbxvt.opt.font = optarg;
			break;
		case 'f': // foreground color
			jbxvt.opt.fg = optarg;
			break;
		case 'S': // scroll history
			jbxvt.scr.sline.max = atoi(optarg);
			break;
		case 's': // use scrollbar
			jbxvt.opt.show_scrollbar=true;
			break;
		case 'v': // version
			jbputs(VERSION "\n");
			quit(0, NULL);
		case 'h': // help
		default:
			jbputs(argv[0]);
			jbputs(" -[");
			jbputs(optstr);
			jbputs("]\n");
			quit(0, NULL);
		}
	}
	return NULL;
}

// constrain rc between 0 and lim, return new value
uint_fast32_t constrain(const int_fast32_t rc, const int_fast32_t lim)
{
	if (rc < 0) // guarantee unsigned
		  return 0;
	else if (rc >= lim) // cap
		  return lim - 1;
	return rc; // falls within range
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

