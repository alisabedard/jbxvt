
/* Copyright 2016, Jeffrey E. Bedard
   Copyright 1992-94, 1997 John Bovey,
   University of Kent at Canterbury. */

#include "ttyinit.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

//  Definitions that enable machine dependent parts of the code.

#ifdef NETBSD
#define POSIX_UTMPX
#include <pwd.h>
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
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <utmpx.h>
static struct utmpx utent; // current utmpx entry
#endif//POSIX_UTMP


static pid_t comm_pid = -1;	// process id of child
static char *tty_name = NULL;	// name of the slave teletype

//  Attempt to create and write an entry to the utmp file
#ifdef POSIX_UTMPX
static void write_utmpx(void)
{
	setutxent();
	utent.ut_type = USER_PROCESS;
	utent.ut_pid = comm_pid;
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
	xcb_disconnect(jbxvt.X.xcb);
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
	static struct termios term;
	term.c_iflag = BRKINT | IGNPAR | ICRNL | IXON;
	term.c_oflag = OPOST | ONLCR;
	// NetBSD needs CREAD
	// Linux needs baud setting
	term.c_cflag = CREAD | CLOCAL | CS8;
#ifdef B38400
	term.c_cflag |= B38400;
#else//!B38400
	term.c_cflag |= B9600;
#endif//B38400
	term.c_lflag = ISIG | IEXTEN | ICANON | ECHO | ECHOE | ECHOK
		| ECHONL;
	term.c_cc[VINTR] = 003;		/* ^C */
	term.c_cc[VQUIT] = 034;		/* ^\ */
	term.c_cc[VERASE] = 0177;	/* DEL */
	term.c_cc[VKILL] = 025;		/* ^U */
	term.c_cc[VEOF] = 004;		/* ^D */
	term.c_cc[VEOL] = 013;		// ^M
	term.c_cc[VSTART] = 021;	/* ^Q */
	term.c_cc[VSTOP] = 023;		/* ^S */
	term.c_cc[VSUSP] = 032;		/* ^Z */
#ifdef VREPRINT
	term.c_cc[VREPRINT] = 022;	/* ^R */
#endif /* VREPRINT */
#ifdef VWERASE
	term.c_cc[VWERASE] = 027;	/* ^W */
#endif /* VWERASE */
#ifdef VLNEXT
	term.c_cc[VLNEXT] = 026;	/* ^V */
#endif /* VLNEXT */
#ifdef VDSUSP
	term.c_cc[VDSUSP] = 031;	/* ^Y */
#endif /* VDSUSP */
#ifdef VDISCARD
	term.c_cc[VDISCARD] = 017;	/* ^O */
#endif /* VDISCARD */
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
	for (int i = 0; i < jbxvt.com.width; i++)
		if (i != ttyfd)
			jb_close(i);
	// for stdin, stderr, stdout:
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
	if (ttyfd > 2)
		jb_close(ttyfd);
	set_ttymodes();
	execvp(argv[0],argv); // Only returns on failure
	exit(1);
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
fd_t run_command(char ** argv)
{
	fd_t ptyfd, ttyfd;
	tty_name = get_pseudo_tty(&ptyfd, &ttyfd);
	if (!tty_name)
		exit(1);
	jb_check(fcntl(ptyfd,F_SETFL,O_NONBLOCK) != 1,
		"Could not set file status flags on pty file descriptor");
	jbxvt.com.width = sysconf(_SC_OPEN_MAX);
	jb_check(jbxvt.com.width != -1, "_SC_OPEN_MAX");

	// Attach relevant signals:
	signal(SIGINT, exit);
	signal(SIGQUIT, exit);
	// grantpt(3) states this is unspecified behavior:
	signal(SIGCHLD, exit);
	atexit(exit_cb);
	comm_pid = fork();
	if (jb_check(comm_pid >= 0, "Could not start session"))
		exit(1);
	if (comm_pid == 0)
		child(argv, ttyfd);
#ifdef POSIX_UTMPX
	write_utmpx();
#endif//POSIX_UTMPX
#ifdef USE_UTEMPTER
	jb_check(utempter_add_record(ptyfd, getenv("DISPLAY")),
		"Could not add utmp record");
#endif//USE_UTEMPTER
	return ptyfd;
}

/*  Tell the teletype handler what size the window is.  Called initially from
 *  the child and after a window size change from the parent.
 */
#if !defined(NETBSD) && !defined(FREEBSD)
#ifdef TIOCSWINSZ
void tty_set_size(const uint8_t width, const uint8_t height)
{
	if (comm_pid < 0)
		return;
	struct winsize wsize = {.ws_row = height, .ws_col = width};
	const fd_t f = comm_pid == 0 ? 0 : jbxvt.com.fd;
	jb_check(ioctl(f, TIOCSWINSZ, &wsize) != 1,
		"Could not set ioctl TIOCSWINSZ");
}
#endif//TIOCSWINSZ
#endif//!NETBSD&&!FREEBSD
