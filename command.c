/* Copyright 2016, Jeffrey E. Bedard
   Copyright 1992-94, 1997 John Bovey,
   University of Kent at Canterbury. */
#include "command.h"
#include "cmdtok.h"
#include "libjb/file.h"
#include "libjb/log.h"
#include "size.h"
#include "xevents.h"
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
//  Definitions that enable machine dependent parts of the code.
#ifdef OPENBSD
#include <sys/ioctl.h>
#include <sys/select.h>
#endif//OPENBSD
#ifdef NETBSD
#define POSIX_UTMPX
#include <pwd.h>
#include <sys/ioctl.h>
#define UTMP_FILE "/var/run/utmp"
#endif//NETBSD
#ifdef _BSD_SOURCE
#include <sys/ttycom.h>
#include <ttyent.h>
#endif//_BSD_SOURCE
#ifdef LINUX
#include <pty.h>
#define POSIX_UTMPX
#endif//LINUX
#ifdef USE_UTEMPTER
#include <utempter.h>
#undef POSIX_UTMPX
#endif//USE_UTEMPTER
#ifdef POSIX_UTMPX
#include <string.h>
#include <utmpx.h>
#endif//POSIX_UTMP
static fd_t command_fd;
static pid_t command_pid;
fd_t jbxvt_get_fd(void)
{
	return command_fd;
}
//  Attempt to create and write an entry to the utmp file
#ifdef POSIX_UTMPX
static void write_utmpx(const pid_t comm_pid, char * tty_name)
{
	setutxent();
	struct utmpx utent = {.ut_type = USER_PROCESS, .ut_pid = comm_pid};
	// + 5 to remove "/dev/"
#define UCP(fld, str) strncpy(utent.fld, str, sizeof(utent.fld))
	UCP(ut_line, tty_name + 5);
	UCP(ut_user, getenv("USER"));
	UCP(ut_host, getenv("DISPLAY"));
#undef UCP
	struct timeval tv;
	// Does not return an error:
	gettimeofday(&tv, NULL);
	utent.ut_tv.tv_sec = tv.tv_sec;
	utent.ut_tv.tv_usec = tv.tv_usec;
	jb_check(pututxline(&utent), "Could not write utmp entry "
		"(are you a member of the utmp group?)");
	endutxent();
}
#endif//POSIX_UTMPX
/*  Acquire a pseudo teletype from the system.  The return value is the
 *  name of the slave part of the pair or NULL if unsucsessful.  If
 *  successful then the master and slave file descriptors are returned
 *  via the arguments.
 */
static char * get_pseudo_tty(int * restrict pmaster,
	int * restrict pslave)
{
	const fd_t mfd = *pmaster = posix_openpt(O_RDWR);
	jb_require(mfd >= 0, "Could not open ptty");
	jb_require(grantpt(mfd) != -1,
		"Could not change mode and owner of slave ptty");
	jb_require(unlockpt(mfd) != -1, "Could not unlock slave ptty");
	char *ttynam = ptsname(mfd);
	jb_require(ttynam, "Could not get tty name");
	*pslave = jb_open(ttynam, O_RDWR);
	return ttynam;
}
//  Initialise the terminal attributes.
static void set_ttymodes(void)
{
	//  Set the terminal using the standard System V termios interface
	static struct termios term = {
		.c_iflag = BRKINT | IGNPAR | ICRNL | IXON,
		.c_oflag = OPOST | ONLCR, .c_cflag = B38400 | CREAD
		| CLOCAL | CS8, .c_lflag = ISIG | IEXTEN | ICANON | ECHO
		| ECHOE | ECHOK | ECHONL, .c_cc = {[VINTR] = 003,
		[VQUIT] = 034, [VERASE] = 0177, [VKILL] = 025,
		[VEOF] = 04, [VEOL] = 013, [VSTART] = 021, [VSTOP] = 023,
		[VSUSP] = 032, [VREPRINT] = 022, [VWERASE] = 027,
		[VLNEXT] = 026, [VDISCARD] = 017}};
	tcsetattr(0, TCSANOW, &term);
	jbxvt_set_tty_size(jbxvt_get_char_size());
}
static void redir(const fd_t target, const fd_t ttyfd)
{
	close(target);
	fcntl(ttyfd, F_DUPFD, 0);
}
static void child(char ** restrict argv, fd_t ttyfd)
{
#if !defined(NETBSD) && !defined(OPENBSD)
	const pid_t pgid = setsid();
#else//NETBSD || OPENBSD
	const pid_t pgid = getsid(getpid());
#endif//!NETBSD && !OPENBSD
	jb_require(pgid >= 0, "Could not open session");
	/*  Having started a new session, we need to establish
	 *  a controlling teletype for it.  On some systems
	 *  this can be done with an ioctl but on others
	 *  we need to re-open the slave tty.  */
#ifdef TIOCSCTTY
	jb_check(ioctl(ttyfd, TIOCSCTTY, 0) != 1,
		"Could not run ioctl TIOCSCTTY");
#else//!TIOCSCTTY
	fd_t i = ttyfd; // save
	ttyfd = jb_open(tty_name, O_RDWR);
	jb_close(i);
#endif//TIOCSCTTY
	// redirect stdin, stderr, and stdout:
	redir(0, ttyfd);
	redir(1, ttyfd);
	redir(2, ttyfd);
	close(ttyfd);
	set_ttymodes();
	execvp(argv[0],argv); // Only returns on failure
	exit(1); // prevent hang
}
/*  Tell the teletype handler what size the window is.
    Called initially from the child and after a window
    size change from the parent.  */
#if !defined(NETBSD) && !defined(FREEBSD)\
	&& defined(HAVE_ASM_GENERIC_IOCTLS_H) && defined(TIOCSWINSZ)
void jbxvt_set_tty_size(const struct JBDim sz)
{
	jb_check(ioctl(jbxvt_get_fd(), TIOCSWINSZ, &(struct winsize){
		.ws_row = sz.row, .ws_col = sz.col}) != 1,
		"Could not set ioctl TIOCSWINSZ");
}
#endif//etc
static void cleanup(void)
{
	LOG("cleanup(), pid: %d, command pid: %d", getpid(), command_pid);
#ifdef USE_UTEMPTER
	utempter_remove_added_record();
#endif//USE_UTEMPTER
	// Make sure child process exits
	kill(command_pid, SIGHUP);
}
#if defined(NETBSD) || defined(OPENBSD)
static void bsd_sigchld(int sig __attribute__((unused)))
{
	static bool called;
	if (!called)
		called = true;
	else
		exit(0);
}
#endif//NETBSD||OPENBSD
static void attach_signals(void)
{
	// Attach relevant signals to ensure cleanup() executed:
	signal(SIGHUP, &exit);
	signal(SIGINT, &exit);
	signal(SIGPIPE, &exit);
	signal(SIGTERM, &exit);
#if defined(NETBSD) || defined(OPENBSD)
	signal(SIGCHLD, &bsd_sigchld);
#else//!NETBSD&&!OPENBSD
	signal(SIGCHLD, &exit);
#endif//NETBSD||OPENBSD
	// Attach exit handler cleanup().
	atexit(cleanup);
}
/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
static fd_t run_command(char ** argv)
{
	fd_t ptyfd, ttyfd;
#ifdef POSIX_UTMPX
	char * tty_name = get_pseudo_tty(&ptyfd, &ttyfd);
#else//!POSIX_UTMPX
	// for FreeBSD:
	get_pseudo_tty(&ptyfd, &ttyfd);
#endif//POSIX_UTMPX
	jb_check(fcntl(ptyfd, F_SETFL, O_NONBLOCK) != 1,
		"Could not set file status flags on pty file descriptor");
	attach_signals();
	jb_require((command_pid = fork()) >= 0, "Could not start session");
	if (command_pid == 0)
		child(argv, ttyfd);
#ifdef POSIX_UTMPX
	write_utmpx(command_pid, tty_name);
#endif//POSIX_UTMPX
#ifdef USE_UTEMPTER
	utempter_add_record(ptyfd, getenv("DISPLAY"));
#endif//USE_UTEMPTER
	return ptyfd;
}
/*  Initialize the command connection.  This should
    be called after the X server connection is established.  */
void jbxvt_init_command_module(char ** restrict argv)
{
	//  Enable the delete window protocol:
	command_fd = run_command(argv);
	jb_require(jbxvt_get_fd() >= 0, "Could not start session");
	jbxvt_init_cmdtok();
}
