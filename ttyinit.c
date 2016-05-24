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
#include "init_display.h"
#include "screen.h"
#include "xsetup.h"

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*  NOTES ON PORTING
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
 * not have termios then you will need to #define BSD_TTY to enable the
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
#define POSIX_PTY
#include <sys/ttycom.h>
#endif//NETBSD

#ifdef FREEBSD
#define POSIX_PTY
#include <termios.h>
#define TTYTAB _PATH_TTYS
#endif//FREEBSD

#ifdef _BSD_SOURCE
#include <sys/ttycom.h>
#include <ttyent.h>
#endif

#ifdef SUNOS5
#include <sys/stropts.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <utmpx.h>
#define SVR4_UTMPX
#define SVR4_PTY
#endif /* SUNOS5 */

#ifdef AIX3
#include <sys/ioctl.h>
#include <sys/stropts.h>
#define SVR4_UTMP
#define BSD_PTY
#endif /* AIX3 */

#ifdef LINUX
#include <pty.h>
#include <stropts.h>
#include <utmp.h>
#define POSIX_PTY
#define SVR4_UTMP
#endif

static pid_t comm_pid = -1;	// process id of child
static char *tty_name = NULL;	// name of the slave teletype
#if defined(BSD_UTMP) || defined(SVR4_UTMP)
static struct utmp utent;	// our current utmp entry
#endif//BSD_UTMP||SVR4_UTMP

#ifdef BSD_UTMP
static int tslot = -1;		// index to our slot in the utmp file
#endif /* BSD_UTMP */


//  Catch a SIGCHLD signal and exit if the direct child has died.
static void catch_child(const int sig __attribute__((unused)))
{
	int status;

	if (wait(&status) == comm_pid)
		quit(status, NULL);

	signal(SIGCHLD,catch_child);
}

// Tidy up the utmp entry etc prior to exiting.
static void tidy_utmp(void)
{
#if defined(SVR4_UTMP) || defined(SVR4_UTMPX)
	setutent();
	if (getutid(&utent) == NULL)
		return;
	utent.ut_type = DEAD_PROCESS;

#ifdef SYS_time
	syscall(SYS_time, &utent.ut_time);
#else//!SYS_time
	time((time_t *)&utent.ut_time);
#endif//SYS_time

	pututline(&utent);
#endif /* SVR4_UTMP || SVR4_UTMPX */
#ifdef SVR4_UTMPX
	updwtmp(WTMP_FILE,&utent);
#endif /* SVR4_UTMPX */

#ifdef BSD_UTMP
	int ut_fd;

	if (tslot < 0)
		return;

#ifdef SYS_open
	if ((ut_fd = syscall(SYS_open, UTMP_FILE, O_WRONLY)) < 0)
#else//!SYS_open
	if ((ut_fd = open(UTMP_FILE,O_WRONLY)) < 0)
#endif//SYS_open
		return;

	memset(&utent,0,sizeof(utent));

#ifdef SYS_lseek
	syscall(SYS_lseek, ut_fd, tslot * sizeof(struct utmp), 0);
#else//!SYS_lseek
	lseek(ut_fd,(long)(tslot * sizeof(struct utmp)),0);
#endif//SYS_lseek

#ifdef SYS_write
	syscall(SYS_write, ut_fd, &utent, sizeof(struct utmp));
#else//!SYS_write
	write(ut_fd,(char *)&utent,sizeof(struct utmp));
#endif//SYS_write

#ifdef SYS_close
	syscall(SYS_close, ut_fd);
#else//!SYS_close
	close(ut_fd);
#endif//SYS_close

#endif /* BSD_UTMP */

#ifdef BSD_PTY
	chmod(tty_name,0666);
#endif /* BSD_PTY */
}

//  Catch a fatal signal and tidy up before quitting
static void catch_sig(const int sig __attribute__((unused)))
{
	quit(0, "Caught signal");
}

#ifdef BSD_UTMP
/*  Look up the tty name in the etc/ttytab file and return a slot number
 *  that can be used to access the utmp file.  We cannot use ttyslot()
 *  because the tty name is not that of fd 0.
 */
static int get_tslot(char * restrict ttynam)
{
	FILE *fs;
	char buf[200], name[200];
	int i;

	if ((fs = fopen(TTYTAB,"r")) == NULL)
		return(-1);
	i = 1;
	while (fgets(buf,200,fs) != NULL) {
		if (*buf == '#')
			continue;
		if (sscanf(buf,"%s",name) != 1)
			continue;
		if (strcmp(ttynam,name) == 0) {
			fclose(fs);
			return(i);
		}
		i++;
	}
	fclose(fs);
	return(-1);
}
#endif /* BSD_UTMP */


//  Attempt to create and write an entry to the utmp file
static void write_utmp(void)
{
#if defined(SVR4_UTMP) || defined(BSD_UTMP)
	struct passwd *pw;
#endif
#ifdef SVR4_UTMP
	utent.ut_type = USER_PROCESS;
	strncpy(utent.ut_id,tty_name + 8,sizeof(utent.ut_id));
	strncpy(utent.ut_line,tty_name + 5,sizeof(utent.ut_line));

#ifdef SYS_getuid
	if((pw = getpwuid(syscall(SYS_getuid))))
#else//!SYS_getuid
	if((pw = getpwuid(getuid())))
#endif//SYS_getuid
		strncpy(utent.ut_name,pw->pw_name,sizeof(utent.ut_name));

	strncpy(utent.ut_host,XDisplayString(jbxvt.X.dpy),
		sizeof(utent.ut_host));

#ifdef SYS_time
	syscall(SYS_time, &utent.ut_time);
#else//!SYS_time
	time((time_t *)&utent.ut_time);
#endif//SYS_time

	pututline(&utent);
	endutent();
#endif /* SVR4_UTMP */

#ifdef SVR4_UTMPX
	struct utmpx utentx;
	int n;

	memset(&utentx,0,sizeof(utentx));
	utentx.ut_type = USER_PROCESS;
	utentx.ut_pid = comm_pid;
	if (sscanf(tty_name,"/dev/pts/%d",&n) != 1) {
		fprintf(stderr, "Can's parse tty name %s\n",tty_name);
		return;
	}
	sprintf(utentx.ut_id,"vt%02x",n);
	sprintf(utentx.ut_line,"pts/%d",n);
	pw = getpwuid(getuid());
	if (pw != NULL)
		strncpy(utentx.ut_name,pw->pw_name,sizeof(utent.ut_name));
	strncpy(utentx.ut_host,XDisplayString(jbxvt.X.dpy),
		sizeof(utentx.ut_host));
	utentx.ut_syslen = strlen(utentx.ut_host) + 1;
	time(&utentx.ut_xtime);
	getutmp(&utentx,&utent);
	pututline(&utent);
	pututxline(&utentx);
	updwtmpx(WTMPX_FILE,&utentx);
	endutent();
#endif /* SVR4_UTMPX */

#ifdef BSD_UTMP
#ifndef UTMP_FILE
#define UTMP_FILE	"/etc/utmp"
#endif /* !UTMP_FILE */

	fd_t ut_fd;

	if ((tslot = get_tslot(tty_name + 5)) < 0)
		fprintf(stderr, "can't locate tty %s in %s",
			tty_name,TTYTAB);

	/*  Attempt to write an entry into the utmp file.
	 */
	if (tslot > 0) {

		if ((ut_fd = open(UTMP_FILE, O_WRONLY)) >= 0) {
			memset(&utent,0,sizeof(utent));
			strncpy(utent.ut_line,tty_name + 5,
				sizeof(utent.ut_line));
			pw = getpwuid(getuid());
			if (pw != NULL)
				strncpy(utent.ut_name,pw->pw_name,
					sizeof(utent.ut_name));
			strncpy(utent.ut_host,
				XDisplayString(jbxvt.X.dpy),
				sizeof(utent.ut_host));
			time(&utent.ut_time);
			lseek(ut_fd,(long)(tslot * sizeof(struct utmp)),0);
			write(ut_fd,(char *)&utent,sizeof(struct utmp));
			close(ut_fd);
		}
	}
#endif /* BSD_UTMP */
}

//  Quit with the status after first removing our entry from the utmp file.
void quit(const int8_t status, const char * restrict msg)
{
	tidy_utmp();
	if(msg) {
#ifdef SYS_write
		  syscall(SYS_write, STDERR_FILENO, msg, strlen(msg));
		  syscall(SYS_write, STDERR_FILENO, "\n", 1);
#else//!SYS_write
		  fputs(msg, stderr);
		  fputc('\n', stderr);
#endif//SYS_write
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
#ifdef BSD_PTY
	char *s3, *s4;
	static char ptyc3[] = "pqrstuvwxyz";
	static char ptyc4[] = "0123456789abcdef";
	static char ptynam[] = "/dev/ptyxx";
	static char ttynam[] = "/dev/ttyxx";

	//  First find a master pty that we can open.
	fd_t mfd = -1;
	for (s3 = ptyc3; *s3 != 0; s3++) {
		for (s4 = ptyc4; *s4 != 0; s4++) {
			ptynam[8] = ttynam[8] = *s3;
			ptynam[9] = ttynam[9] = *s4;
#ifdef SYS_open
			if ((mfd = syscall(SYS_open, ptynam,O_RDWR)) >= 0) {
#else//!SYS_open
			if ((mfd = open(ptynam,O_RDWR)) >= 0) {
#endif//SYS_open

#ifdef SYS_geteuid
				if (syscall(SYS_geteuid) == 0
#else//!SYS_geteuid
				if (geteuid() == 0
#endif//SYS_geteuid

#ifdef SYS_access
					|| syscall(SYS_access, ttynam,
				       		R_OK|W_OK) == 0)
#else//!SYS_access
					|| access(ttynam,R_OK|W_OK) == 0)
#endif//SYS_access
					break;
				else {
#ifdef SYS_close
					syscall(SYS_close, mfd);
#else//!SYS_close
					close(mfd);
#endif//SYS_close
					mfd = -1;
				}
			}
		}
		if (mfd >= 0)
			break;
	}
#endif /* BSD_PTY */

#ifdef POSIX_PTY
	const fd_t mfd = posix_openpt(O_RDWR);
#endif//POSIX_PTY
	if (mfd < 0) 
		  quit(1, "Cannot open pseudo tty");

#ifdef POSIX_PTY
	grantpt(mfd);
	unlockpt(mfd);
	char *ttynam = ptsname(mfd);
#endif//POSIX_PTY

#ifdef SYS_open
	const fd_t sfd = syscall(SYS_open, ttynam, O_RDWR);
#else//!SYS_open
	const fd_t sfd = open((const char *)ttynam,O_RDWR);
#endif//SYS_open

	if (sfd < 0)
		quit(1, "Cannot open slave tty");

#if defined(POSIX_PTY) && defined(I_PUSH)
#ifdef SYS_ioctl
	syscall(SYS_ioctl, sfd, I_PUSH, "ptem");
	syscall(SYS_ioctl, sfd, I_PUSH, "ldterm");
#else//!SYS_ioctl
	ioctl(sfd, I_PUSH, "ptem");
	ioctl(sfd, I_PUSH, "ldterm");
#endif//SYS_ioctl
#endif//POSIX_PTY&&I_PUSH

	*pslave = sfd;
	*pmaster = mfd;
	return ttynam;
}

//  Initialise the terminal attributes.
static void set_ttymodes(void)
{
	uint16_t width, height;

#ifndef BSD_TTY
	//  Set the terminal using the standard System V termios interface
	static struct termios term;

	term.c_iflag = BRKINT | IGNPAR | ICRNL | IXON;

	term.c_oflag = OPOST | ONLCR;

	// NetBSD needs CREAD
	// Linux needs B9600
	term.c_cflag = CREAD | CLOCAL | B9600 | CS8;

	term.c_lflag = ISIG | IEXTEN | ICANON | ECHO | ECHOE | ECHOK;

	term.c_cc[VINTR] = 003;		/* ^C */
	term.c_cc[VQUIT] = 034;		/* ^\ */
	term.c_cc[VERASE] = 0177;	/* DEL */
	term.c_cc[VKILL] = 025;		/* ^U */
	term.c_cc[VEOF] = 004;		/* ^D */
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

	tcsetattr(0,TCSANOW,&term);

#else /* BSD_TTY */
	/* Use sgtty rather than termios interface to configure the terminal
	 */
	int ldisc, lmode;
	struct sgttyb tty;
	struct tchars tc;
	struct ltchars ltc;

#ifdef NTTYDISC
	ldisc = NTTYDISC;
	(void)ioctl(0,TIOCSETD,&ldisc);
#endif /* NTTYDISC */
	tty.sg_ispeed = B9600;
	tty.sg_ospeed = B9600;
	tty.sg_erase = 0177;
	tty.sg_kill = 025;		/* ^U */
	tty.sg_flags = CRMOD | ECHO | EVENP | ODDP;
	(void)ioctl(0,TIOCSETP,&tty);

	tc.t_intrc = 003;		/* ^C */
	tc.t_quitc = 034;		/* ^\ */
	tc.t_startc = 021;		/* ^Q */
	tc.t_stopc = 023;		/* ^S */
	tc.t_eofc =  004;		/* ^D */
	tc.t_brkc = -1;
	(void)ioctl(0,TIOCSETC,&tc);

	ltc.t_suspc = 032;		/* ^Z */
	ltc.t_dsuspc = 031;		/* ^Y */
	ltc.t_rprntc = 022;		/* ^R */
	ltc.t_flushc = 017;		/* ^O */
	ltc.t_werasc = 027;		/* ^W */
	ltc.t_lnextc = 026;		/* ^V */
	(void)ioctl(0,TIOCSLTC,&ltc);

	lmode = LCRTBS | LCRTERA | LCTLECH | LPASS8 | LCRTKIL;
	(void)ioctl(0,TIOCLSET,&lmode);
#endif// BSD_TTY

	scr_get_size(&width,&height);
	tty_set_size(width,height);
}

static void child(char ** restrict argv, fd_t ttyfd)
{
#ifndef NETBSD

#ifdef SYS_setsid
	const pid_t pgid = syscall(SYS_setsid);
#else//!SYS_setsid
	const pid_t pgid = setsid();
#endif//SYS_setsid

#else//NETBSD
#if defined(SYS_getsid) && defined(SYS_getpid)
	const pid_t pgid = syscall(SYS_getsid, syscall(SYS_getpid));
#else//!(SYS_getsid&&SYS_getpid)
	const pid_t pgid = getsid(getpid());
#endif//SYS_getsid&&SYS_getpid
#endif//!NETBSD

	if(pgid < 0)
		  quit(1, "Cannot create new session");

	/*  Having started a new session, we need to establish
	 *  a controlling teletype for it.  On some systems
	 *  this can be done with an ioctl but on others
	 *  we need to re-open the slave tty.  */
#ifdef TIOCSCTTY

#ifdef SYS_ioctl
	syscall(SYS_ioctl, ttyfd, TIOCSCTTY, 0);
#else//!SYS_ioctl
	(void)ioctl(ttyfd,TIOCSCTTY,0);
#endif//SYS_ioctl

#else//!TIOCSCTTY

	fd_t i = ttyfd;

#ifdef SYS_open
	ttyfd = syscall(SYS_open, tty_name, O_RDWR);
#else//!SYS_open
	ttyfd = open(tty_name, O_RDWR);
#endif//SYS_open

	if (ttyfd < 0)
		  quit(1, "Cannot open tty");

#ifdef SYS_close
	syscall(SYS_close, i);
#else//!SYS_close
	close(i);
#endif//SYS_close

#endif//TIOCSCTTY

#ifdef SYS_getuid
	const uid_t uid = syscall(SYS_getuid);
#else//!SYS_getuid
	const uid_t uid = getuid();
#endif//SYS_getuid

	struct group * gr = getgrnam("tty");
	const gid_t gid = gr ? (int)gr->gr_gid : -1;

#ifdef SYS_fchown
	syscall(SYS_fchown, ttyfd, uid, gid);
	syscall(SYS_fchmod, ttyfd, 0620);
#else//!SYS_fchown
	fchown(ttyfd,uid,gid);
	fchmod(ttyfd, 0620);
#endif//SYS_fchown

	for (int i = 0; i < jbxvt.com.width; i++)
		  if (i != ttyfd)
#ifdef SYS_close
			    syscall(SYS_close, i);
#else//!SYS_close
			    close(i);
#endif//SYS_close

	// for stdin, stderr, stdout:
#ifdef SYS_fcntl
	syscall(SYS_fcntl, ttyfd, F_DUPFD, 0);
	syscall(SYS_fcntl, ttyfd, F_DUPFD, 0);
	syscall(SYS_fcntl, ttyfd, F_DUPFD, 0);
#else//!SYS_fcntl
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
	fcntl(ttyfd, F_DUPFD, 0);
#endif//SYS_fcntl

	if (ttyfd > 2) {
#ifdef SYS_close
		  syscall(SYS_close, ttyfd);
#else//!SYS_close
		  close(ttyfd);
#endif//SYS_close
	}

#ifdef BSD_TTY

#ifdef SYS_ioctl
	syscall(SYS_ioctl, 0, TIOCSPGRP, &pgid);
#else//!SYS_ioctl
	ioctl(0, TIOCSPGRP, (char *)&pgid);
#endif//SYS_ioctl

#ifdef SYS_setpgid
	syscall(SYS_setpgid, 0, pgid);
#else//!SYS_setpgid
        setpgid(0, pgid);
#endif//SYS_setpgid
#endif /* BSD_TTY */
	set_ttymodes();

#ifdef SYS_setuid
	syscall(SYS_setuid, uid);
#else//!SYS_setuid
	setuid(uid);
#endif//SYS_setuid

	execvp(argv[0],argv);
	quit(1, "Cannot execute command");
}

/*  Run the command in a subprocess and return a file descriptor for the
 *  master end of the pseudo-teletype pair with the command talking to
 *  the slave.  */
int run_command(char ** argv)
{
	fd_t ptyfd, ttyfd;
	tty_name = get_pseudo_tty(&ptyfd, &ttyfd);
	if (!tty_name)
		  return -1;
#ifdef SYS_fcntl
	syscall(SYS_fcntl, ptyfd, F_SETFL, O_NONBLOCK);
#else//!SYS_fcntl
	fcntl(ptyfd,F_SETFL,O_NONBLOCK);
#endif//SYS_fcntl

	jbxvt.com.width = sysconf(_SC_OPEN_MAX);
	for (uint8_t i = 1; i <= 15; i++)
		signal(i, catch_sig);

#ifdef SYS_fork
	comm_pid = syscall(SYS_fork);
#else//!SYS_fork
	comm_pid = fork();
#endif//SYS_fork

	if (comm_pid < 0)
		  quit(1, "Cannot fork");
#ifdef DEBUG
	fprintf(stderr, "command: %s, pid: %d\n", argv[0], comm_pid);
#endif//DEBUG
	if (comm_pid == 0)
		  child(argv, ttyfd);
	signal(SIGCHLD,catch_child);
	write_utmp();
	return(ptyfd);
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

#ifdef SYS_ioctl
	syscall(SYS_ioctl, f, TIOCSWINSZ, &wsize);
#else//!SYS_ioctl
	ioctl(f, TIOCSWINSZ, &wsize);
#endif//SYS_ioctl
}
#endif//TIOCSWINSZ
#endif//!NETBSD&&!FREEBSD
