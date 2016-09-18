# Copyright 2016, Jeffrey E. Bedard
#CC=clang
#include debug.mk
exe=jbxvt
PREFIX=/usr

include config.mk

CFLAGS+=-DUSE_LIKELY
CFLAGS+=-D_XOPEN_SOURCE=700 --std=c11
CFLAGS+=-Wall -Wextra

LIBS+=-lxcb -lxcb-keysyms -lgc
LIBS+=-Llibjb -ljb

OBJS=jbxvt.o lookup_key.o paint.o change_selection.o cmdtok.o
OBJS+=cursor.o init_display.o repaint.o save_selection.o scr_move.o
OBJS+=sbar.o scr_erase.o selex.o scr_edit.o command.o selection.o
OBJS+=selreq.o scr_reset.o scr_string.o screen.o scroll.o selend.o 
OBJS+=xevents.o xsetup.o xvt.o handle_sgr.o dec_reset.o show_selection.o

$(exe): $(OBJS)
	cd libjb && $(MAKE) CC="${CC}" CFLAGS="${CFLAGS}"
	$(CC) -o $(exe) $(OBJS) $(CFLAGS) $(LIBS)
	strip -o $(exe).tmp $(exe)
	ls -l $(exe).tmp >> sz.log
	rm -f $(exe).tmp
	tail -n 5 sz.log

bindest=$(DESTDIR)$(PREFIX)/bin
docdest=$(DESTDIR)$(PREFIX)/share/man/man1

install:
	install -d $(bindest)
	install $(exe) $(bindest)
	install -d $(docdest)
	install $(exe).1 $(docdest)
clean:
	rm -f $(exe) *.o
	cd libjb && make clean

d: # DEBUG build
	CFLAGS='-DDEBUG -ggdb -O0 -Werror' make -j8

f: # Optimized build
	$(MAKE) clean
	CFLAGS='-Ofast -march=native -flto' make -j8

s: # Tiny build
	$(MAKE) clean
	CFLAGS='-Os -march=native -flto' make -j8
#EOF
