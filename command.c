/* Copyright 2016, Jeffrey E. Bedard
   Copyright 1992-94, 1997 John Bovey,
   University of Kent at Canterbury. */

#include "command.h"

#include "jbxvt.h"
#include "xevents.h"

#include <fcntl.h>
#include <gc.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

//  Definitions that enable machine dependent parts of the code.

#ifdef NETBSD
#define POSIX_UTMPX
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/ttycom.h>
#define UTMP_FILE "/var/run/utmp"
#endif//NETBSD

#ifdef FREEBSD
#include <termios.h>
#endif//FREEBSD

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

//  Attempt to create and write an entry to the utmp file
#ifdef POSIX_UTMPX
static void write_utmpx(const pid_t comm_pid, char * tty_name)
{
	setutxent();
	struct utmpx utent = {.ut_type = USER_PROCESS, .ut_pid = comm_pid};
	// + 5 to remove "/dev/"
	strncpy(utent.ut_line, tty_name + 5, sizeof(utent.ut_line));
	strncpy(utent.ut_user, getenv("USER"), sizeof(utent.ut_user));
	strncpy(utent.ut_host, getenv("DISPLAY"), sizeof(utent.ut_host));
	struct timeval tv;
	// Does not return an error:
	gettimeofday(&tv, NULL);
	utent.ut_tv.tv_sec = tv.tv_sec;
	utent.ut_tv.tv_usec = tv.tv_usec;
	jb_check(pututxline(&utent), "Could not write utmp entry.");
	endutxent();
}
#endif//POSIX_UTMPX

// Put all clean-up tasks here:
static void exit_cb(void)
{
#ifdef USE_UTEMPTER
	jb_check(utempter_remove_added_record(),
		"Could not remove utmp record");
#endif//USE_UTEMPTER
}

/*  Acquire a pseudo teletype from the system.  The return value is the
 *  name of the slave part of the pair or NULL if unsucsessful.  If
 *  successful then the master and slave file descriptors are returned
 *  via the arguments.
 */
static char * get_pseudo_tty(int * restrict pmaster, int * restrict pslave)
{
	const fd_t mfd = posix_openpt(O_RDWR);
	bool e = false; // error flag
	e |= jb_check(mfd >= 0, "Could not open ptty");
	e |= jb_check(grantpt(mfd) != -1,
		"Could not change mode and owner of slave ptty");
	e |= jb_check(unlockpt(mfd) != -1, "Could not unlock slave ptty");
	char *ttynam = ptsname(mfd);
	e |= jb_check(ttynam, "Could not get tty name");
	if (e) // error condition
		exit(1);
	*pslave = jb_open(ttynam, O_RDWR);
	*pmaster = mfd;
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
	tty_set_size(jbxvt.scr.chars.width, jbxvt.scr.chars.height);
}

static void child(char ** restrict argv, fd_t ttyfd)
{
#ifndef NETBSD
	const pid_t pgid = setsid();
#else//NETBSD
	const pid_t pgid = getsid(getpid());
#endif//!NETBSD
	if (jb_check(pgid >= 0, "Could not open session"))
		exit(1);
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
	jb_close(0);
	jb_close(1);
	jb_close(2);
	// for stdin, stderr, stdout:
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
	set_ttymodes();
	execvp(argv[0],argv); // Only returns on failure
	exit(1);
}


/*  Tell the teletype handler what size the window is.  Called initially from
 *  the child and after a window size change from the parent.
 */
#if !defined(NETBSD) && !defined(FREEBSD)
#ifdef TIOCSWINSZ
void tty_set_size(const uint8_t width, const uint8_t height)
{
	struct winsize wsize = {.ws_row = height, .ws_col = width};
	jb_check(ioctl(jbxvt.com.fd, TIOCSWINSZ, &wsize) != 1,
		"Could not set ioctl TIOCSWINSZ");
}
#endif//TIOCSWINSZ
#endif//!NETBSD&&!FREEBSD

static void attach_signals(void)
{
	// Attach relevant signals:
	signal(SIGINT, exit);
	signal(SIGQUIT, exit);
	// grantpt(3) states this is unspecified behavior:
	signal(SIGCHLD, exit);
	atexit(exit_cb);
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
static fd_t run_command(char ** argv)
{
	fd_t ptyfd, ttyfd;
	char * tty_name = get_pseudo_tty(&ptyfd, &ttyfd);
	if (!tty_name)
		exit(1);
	jb_check(fcntl(ptyfd, F_SETFL, O_NONBLOCK) != 1,
		"Could not set file status flags on pty file descriptor");
	// +1 to allow for X fd
	jbxvt.com.width = ptyfd + 1;
	attach_signals();
	const pid_t comm_pid = fork();
	if (jb_check(comm_pid >= 0, "Could not start session"))
		exit(1);
	if (comm_pid == 0)
		child(argv, ttyfd);
#ifdef POSIX_UTMPX
	write_utmpx(comm_pid, tty_name);
#endif//POSIX_UTMPX
#ifdef USE_UTEMPTER
	jb_check(utempter_add_record(ptyfd, getenv("DISPLAY")),
		"Could not add utmp record");
#endif//USE_UTEMPTER
	return ptyfd;
}

/*  Initialize the command connection.  This should
    be called after the X server connection is established.  */
void init_command(char ** restrict argv)
{
	//  Enable the delete window protocol:
	wm_del_win();
	jbxvt.com.xfd = xcb_get_file_descriptor(jbxvt.X.xcb);
	jbxvt.com.fd = run_command(argv);
	if (jb_check(jbxvt.com.fd >= 0, "Could not start session"))
		exit(1);
	struct JBXVTCommandContainer * b = &jbxvt.com.buf,
				     * s = &jbxvt.com.stack;
	b->data = b->next = b->top = GC_MALLOC(COM_BUF_SIZE);
	s->data = s->top = GC_MALLOC(COM_PUSH_MAX);
}

//  Push an input character back into the input queue.
void push_com_char(const uint8_t c)
{
	if (jbxvt.com.stack.top < jbxvt.com.stack.data
		+ COM_PUSH_MAX)
		*jbxvt.com.stack.top++ = c;
}

/*  Send printf formatted output to the command.
    Only used for small ammounts of data.  */
char * cprintf(char *fmt, ...)
{
	va_list args;
	va_start(args,fmt);
	static char buf[32];
	// + 1 to include \0 terminator.
	const int l = vsnprintf(buf, sizeof(buf), fmt, args) + 1;
	va_end(args);
	jbxvt.com.send_nxt = (uint8_t *)buf;
	jbxvt.com.send_count = l;
	return buf;
}

