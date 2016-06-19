#CFLAGS+=-Os
#CFLAGS+=-Werror
#CFLAGS+=-flto # doesn't work with clang
#CFLAGS=-O0
#CFLAGS+=-ggdb
#CFLAGS+=-DDEBUG
#CFLAGS+=-DTK_DEBUG
#CC=clang

exe=jbxvt
PREFIX=/usr

# Uncomment for NetBSD:
#CFLAGS+=-DNETBSD -D_NETBSD_SOURCE -D_BSD_SOURCE
#CFLAGS+=-Wno-missing-field-initializers
#CFLAGS+=-I/usr/X11R7/include
#LIBS+=-L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib
#PREFIX=/usr/local

# Uncomment for FreeBSD:
#CFLAGS+=-DFREEBSD -D_BSD_SOURCE -D__BSD_VISIBLE
#CFLAGS+=-I/usr/local/include
#LIBS+=-L/usr/local/lib
#PREFIX=/usr/local
#LIBS+=-lutempter

# Uncomment for GNU/Linux:
CFLAGS+=-DLINUX -D_GNU_SOURCE
LIBS+=-lutempter

#-------------------------------

CFLAGS+=-DUSE_LIKELY
LIBS+=-lxcb -lxcb-keysyms -lX11

#-------------------------------

OBJS=jbxvt.o
OBJS+=change_offset.o change_selection.o cmdtok.o color.o command.o
OBJS+=cursor.o init_display.o repaint.o save_selection.o
OBJS+=sbar.o scr_delete_characters.o scr_erase.o scr_extend_selection.o
OBJS+=scr_insert_characters.o scr_move.o scr_refresh.o
OBJS+=scr_request_selection.o scr_reset.o scr_string.o screen.o
OBJS+=scroll.o selcmp.o selection.o show_selection.o ttyinit.o
OBJS+=wm_del_win.o xevents.o xsetup.o xvt.o handle_sgr.o
CFLAGS+=-D_XOPEN_SOURCE=700 --std=c99
CFLAGS+=-Wall -Wextra
$(exe): $(OBJS)
	$(CC) -o $(exe) $(OBJS) $(CFLAGS) $(LIBS)
	strip -o $(exe).tmp $(exe)
	ls -l $(exe).tmp >> sz.log
	rm -f $(exe).tmp
	tail sz.log
bindest=$(DESTDIR)$(PREFIX)/bin
install:
	install -d $(bindest)
	install $(exe) $(bindest)
clean:
	rm -f $(exe) *.o

# DO NOT DELETE THIS LINE -- make depend depends on it.
