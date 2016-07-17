#CFLAGS+=-Os
#CFLAGS+=-Werror
#CFLAGS+=-flto # doesn't work with clang
#CFLAGS=-O0
#CFLAGS+=-ggdb
#CFLAGS+=-DDEBUG
#CFLAGS+=-DGC_DEBUG
#CC=clang
CFLAGS+=-Wall -Wextra

exe=jbxvt
PREFIX=/usr

# Uncomment for NetBSD:
#CFLAGS+=-DNETBSD -D_NETBSD_SOURCE -D_BSD_SOURCE
#CFLAGS+=-Wno-missing-field-initializers
#CFLAGS+=-I/usr/X11R7/include -I/usr/pkg/include
#LIBS+=-L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib
#LIBS+=-L/usr/pkg/lib -Wl,-R/usr/pkg/lib
#PREFIX=/usr/local

# Uncomment for FreeBSD:
#CFLAGS+=-DFREEBSD -D_BSD_SOURCE -D__BSD_VISIBLE
#CFLAGS+=-I/usr/local/include
#LIBS+=-L/usr/local/lib
#PREFIX=/usr/local
#LIBS+=-lutempter

# Uncomment for GNU/Linux:
CFLAGS+=-DLINUX -D_GNU_SOURCE

# Uncomment to use libutempter for utmp access
#CFLAGS+=-DUSE_UTEMPTER
#LIBS+=-lutempter

#-------------------------------

CFLAGS+=-DUSE_LIKELY
LIBS+=-lxcb -lxcb-keysyms -lgc

#-------------------------------

OBJS=jbxvt.o lookup_key.o paint.o
OBJS+=change_offset.o change_selection.o cmdtok.o color.o command.o
OBJS+=cursor.o init_display.o repaint.o save_selection.o scr_move.o
OBJS+=sbar.o scr_erase.o scr_extend_selection.o scr_edit.o
OBJS+=scr_request_selection.o scr_reset.o scr_string.o screen.o
OBJS+=scroll.o selcmp.o selection.o show_selection.o ttyinit.o
OBJS+=wm_del_win.o xevents.o xsetup.o xvt.o handle_sgr.o dec_reset.o
LIBS+= -Llibjb -ljb
CFLAGS+=-D_XOPEN_SOURCE=700 --std=c99
CFLAGS+=-Wall -Wextra

all:
	cd libjb && make
	make $(exe)

$(exe): $(OBJS)
	$(CC) -o $(exe) $(OBJS) $(CFLAGS) $(LIBS)
	strip -o $(exe).tmp $(exe)
	ls -l $(exe).tmp >> sz.log
	rm -f $(exe).tmp
	tail -n 5 sz.log

bindest=$(DESTDIR)$(PREFIX)/bin
install:
	install -d $(bindest)
	install $(exe) $(bindest)
clean:
	rm -f $(exe) *.o
	cd libjb && make clean

# DO NOT DELETE THIS LINE -- make depend depends on it.
