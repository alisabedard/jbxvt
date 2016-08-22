CFLAGS+=-Wall -Wextra -ggdb
exe=jbxvt
PREFIX=/usr

# Uncomment for NetBSD:
#CFLAGS+=-DNETBSD -D_NETBSD_SOURCE -D_BSD_SOURCE
#CFLAGS+=-Wno-missing-field-initializers
#CFLAGS+=-I/usr/pkg/include
#CFLAGS+=-I/usr/X11R7/include 
#LIBS+=-L/usr/pkg/lib -Wl,-R/usr/pkg/lib
#LIBS+=-L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib
#LIBS+=-L/usr/X11R6/lib -Wl,-R/usr/X11R6/lib
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

OBJS=jbxvt.o lookup_key.o paint.o change_selection.o cmdtok.o
OBJS+=cursor.o init_display.o repaint.o save_selection.o scr_move.o
OBJS+=sbar.o scr_erase.o selex.o scr_edit.o command.o selection.o
OBJS+=selreq.o scr_reset.o scr_string.o screen.o scroll.o selend.o 
OBJS+=xevents.o xsetup.o xvt.o handle_sgr.o dec_reset.o show_selection.o
LIBS+= -Llibjb -ljb
CFLAGS+=-D_XOPEN_SOURCE=700 --std=c11
CFLAGS+=-Wall -Wextra

$(exe): $(OBJS)
	cd libjb && make CFLAGS="${CFLAGS}"
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

d: # DEBUG build
	CFLAGS='-DDEBUG -ggdb -O0 -Werror' make -j8

f: # Optimized build
	make clean
	CFLAGS='-Ofast -march=native -flto' make -j8

s: # Tiny build
	make clean
	CFLAGS='-Os -march=native -flto' make -j8

# DO NOT DELETE THIS LINE -- make depend depends on it.
