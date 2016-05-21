#include "jbxvt.h"

#include "config.h"
#include "init_display.h"
#include "xsetup.h"
#include "xvt.h"

#include <stdlib.h>
#include <string.h>

struct JBXVT jbxvt;

// returns new command value
static char ** process_exec_flag(int * restrict argc, char ** argv)
{
	/*  Look for a -e flag and if it is there use it to initialise
	 *  the command and its arguments.  */
	uint8_t i;
	char ** com_argv = NULL;

	for (i = 0; i < *argc; i++)
		  if (strcmp(argv[i],"-e") == 0)
			    break;
	if (i < *argc - 1) {
		argv[i] = NULL;
		com_argv = argv + i + 1;
		*argc = i;
	}

	return com_argv;
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
int main(int argc, char ** argv)
{
	char ** com_argv = process_exec_flag(&argc, argv);
	
	init_display(argc,argv);
	map_window();
	char *shell_argv[2]; // here to not lose scope.
	if(!com_argv) {
		shell_argv[0] = getenv("SHELL");
		shell_argv[1] = NULL;
		com_argv = shell_argv;
	}
#ifdef DEBUG
	fprintf(stderr, "Command: %s\n", com_argv[0]);
#endif//DEBUG
	setenv("TERM", TERM_ENV, true);
	init_command(com_argv);
	jbxvt_app_loop();

	return 0;
}
