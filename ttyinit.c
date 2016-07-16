// Copyright 2016, Jeffrey E. Bedard
/*  Copyright 1992-94, 1997 John Bovey, University of Kent at Canterbury.
 *
 *  Redistribution and use in source code and/or executable forms, with
 *  or without modification, are permitted provided that the following
 *  condition is met:
 *
 *  Any redistribution must retain the above copyright notice, this
 *  condition and the following disclaimer, either as part of the
 *  program source code included in the redistribution or in human-
 *  readable materials provided with the redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS".  Any express or implied
 *  warranties concerning this software are disclaimed by the copyright
 *  holder to the fullest extent permitted by applicable law.  In no
 *  event shall the copyright-holder be liable for any damages of any
 *  kind, however caused and on any theory of liability, arising in any
 *  way out of the use of, or inability to use, this software.
 *
 *  -------------------------------------------------------------------
 *
 *  In other words, do not misrepresent my work as your own work, and
 *  do not sue me if it causes problems.  Feel free to do anything else
 *  you wish with it.
 */

#include "ttyinit.h"

#include "jbxvt.h"
#include "log.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/*  NOTES ON PORTING -- OBSOLETE, but kept for reference
 *
 * Almost all the non-portable parts of xvt are concerned with setting up
 * and configuring the pseudo-teletype (pty) link that connects xvt itself
 * to the commands running in its window.  In practice, there are four
 * different tasks that need to be done to get a pty up and running and
 * each of these tasks can be done in different ways, depending on the
 * flavour of UNIX.  The four tasks are:
 *
 * Obtaining a free pty
 * --------------------
 * This is handled in the function get_pseudo_teletype.  On BSD flavours
 * of UNIX, the directory /dev contains a block of pty master devices,
 * with names pty??, and a matching set of slave teletype devices with the
 * initial 'p' replaced by a 't'.  The way to find a free pseudoteletype
 * pair is to try opening pty devices, one by one, until one is opened
 * successfully.  Then the matching tty device is opened to form the other
 * end of the pair.  The alternative (SVR4?) approach is to open /dev/ptmx
 * and then use a sequence of system calls to request and initialise the
 * pty pair.  Either of the two approaches are selected by defining one of
 * BSD_PTY and SVR4_PTY, but the only system I have found which supports
 * SVR4_PTY is SunOS 5.
 *
 * Creation of a controlling teletype
 * ----------------------------------
 * After the fork(), the slave tty needs to be made into the controlling
 * teletype for the child process (which will eventually exec the command
 * program, usually a shell).  On some systems this is done with a
 * TIOCSCTTY ioctl after calling setsid() to create a new session.  On
 * most other systems, the procedure is to call setsid() and then re-open
 * the tty.  This latter approach is the default - if your system uses
 * TIOCSCTTY then #define SCTTY_IOCTL.
 *
 * Configure the teletype
 * ----------------------
 * This is handled in set_ttymodes().  SVR4 systems use the termios
 * structure and the tcsetattr() system call whereas BSD systems use the
 * sgttyb structure and TIOC ioctls.  In the systems I have ported to,
 * termios is prevalent and so this is the default.  If your system does
 * not have termios then you will need to #define BSD_PTY to enable the
 * use of sgttyb instead.
 *
 * Adding an entry to utmp
 * -----------------------
 * Xvt needs to add an entry to /etc/utmp if the session is to be visible
 * to programs like finger and who.  On BSD systems this involved finding
 * the correct position in the utmp file and then using write(2) to insert
 * an entry.  On SVR4 systems the work is done by pututline(3) and
 * family.  SunOS 5 has a utmp extension which stores additional
 * information about each session.  The relevent #defines are SVR4_UTMP,
 * BSD_UTMP and SVR4_UTMPX.  If you don't need utmp entries then you don't
 * need to define any of these.
 */

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
	gettimeofday(&tv, NULL);
	utent.ut_tv.tv_sec = tv.tv_sec;
	utent.ut_tv.tv_usec = tv.tv_usec;
	if(!pututxline(&utent)) {
		perror("Could not write UTMPX entry!");
	}
	endutxent();
}
#endif//POSIX_UTMPX

//  Quit with the status after first removing our entry from the utmp file.
void quit(const int8_t status, const char * restrict msg)
{
#ifdef USE_UTEMPTER
	utempter_remove_added_record();
#endif//USE_UTEMPTER
	if(msg) {
		jbputs(msg);
		jbputs("\n");
	}
	exit(status);
}

/*  Acquire a pseudo teletype from the system.  The return value is the
 *  name of the slave part of the pair or NULL if unsucsessful.  If
 *  successful then the master and slave file descriptors are returned
 *  via the arguments.
 */
static char * get_pseudo_tty(int * restrict pmaster, int * restrict pslave)
{
	const fd_t mfd = posix_openpt(O_RDWR);
	if (mfd < 0)
		  quit(1, WARN_RES RES_TTY);
	grantpt(mfd);
	unlockpt(mfd);
	char *ttynam = ptsname(mfd);
	const fd_t sfd = open((const char *)ttynam,O_RDWR);
	if (sfd < 0)
		quit(1, WARN_RES RES_TTY);
	*pslave = sfd;
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
	term.c_lflag = ISIG | IEXTEN | ICANON;
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

static void close_checked(const int fd)
{
	if (close(fd) != -1)
		return;
	const uint8_t sz = 32;
	char buf[sz];
	snprintf(buf, sz, "Could not close fd %d", fd);
	perror(buf);
}

static void child(char ** restrict argv, fd_t ttyfd)
{
#ifndef NETBSD
	const pid_t pgid = setsid();
#else//NETBSD
	const pid_t pgid = getsid(getpid());
#endif//!NETBSD
	if(pgid < 0)
		quit(1, WARN_RES RES_SSN);

	/*  Having started a new session, we need to establish
	 *  a controlling teletype for it.  On some systems
	 *  this can be done with an ioctl but on others
	 *  we need to re-open the slave tty.  */

	int r; // for return value checking
#ifdef TIOCSCTTY
	r = ioctl(ttyfd,TIOCSCTTY,0);
	if (r == 1) {
		perror("ioctl TIOCSCTTY");
		r = 0;
	}
#else//!TIOCSCTTY
	fd_t i = ttyfd;
	ttyfd = open(tty_name, O_RDWR);
	if (ttyfd < 0)
		quit(1, WARN_RES RES_SSN);
	close_checked(i);
#endif//TIOCSCTTY
	for (int i = 0; i < jbxvt.com.width; i++)
		if (i != ttyfd)
			close_checked(i);
	// for stdin, stderr, stdout:
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
	if (ttyfd > 2)
		close_checked(ttyfd);
	set_ttymodes();
	execvp(argv[0],argv); // Only returns on failure
	quit(1, WARN_RES RES_SSN);
}

// wrap quit for signal function pointer
__attribute__((noreturn))
static void sigquit(int sig __attribute__((unused)))
{
	quit(0, NULL);
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
fd_t run_command(char ** argv)
{
	fd_t ptyfd, ttyfd;
	tty_name = get_pseudo_tty(&ptyfd, &ttyfd);
	if (!tty_name)
		return -1;
	fcntl(ptyfd,F_SETFL,O_NONBLOCK);
	jbxvt.com.width = sysconf(_SC_OPEN_MAX);
	// Attach relevant signals:
	signal(SIGINT, sigquit);
	signal(SIGQUIT, sigquit);
	comm_pid = fork();
	if (comm_pid < 0)
		quit(1, WARN_RES RES_SSN);
	if (comm_pid == 0)
		child(argv, ttyfd);
	signal(SIGCHLD, sigquit);
#ifdef POSIX_UTMPX
	write_utmpx();
#endif//POSIX_UTMPX
#ifdef USE_UTEMPTER
	utempter_add_record(ptyfd, getenv("DISPLAY"));
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
	struct winsize wsize = {.ws_row = height, .ws_col = width};
	ioctl(jbxvt.com.fd, TIOCSWINSZ, &wsize);
}
#endif//TIOCSWINSZ
#endif//!NETBSD&&!FREEBSD
