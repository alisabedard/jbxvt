CFLAGS+=-Os
CFLAGS+=-Werror
#CFLAGS+=-march=native
#CFLAGS+=-flto # doesn't work with clang
#CFLAGS+=-ggdb
#CFLAGS+=-DDEBUG
#CFLAGS+=-DTK_DEBUG
#CC=clang

exe=jbxvt

# Uncomment for NetBSD:
#CFLAGS+=-DNETBSD
#CFLAGS+=-I/usr/X11R7/include
#LIBS+=-L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib


# Uncomment for GNU/Linux:
CFLAGS+=-DLINUX

LIBS+=-lX11

OBJS=change_offset.o change_selection.o cmdtok.o color.o command.o
OBJS+=cursor.o init_display.o repaint.o repair_damage.o save_selection.o
OBJS+=sbar.o scr_delete_characters.o scr_erase.o scr_extend_selection.o
OBJS+=scr_insert_characters.o scr_move.o scr_refresh.o
OBJS+=scr_request_selection.o scr_reset.o scr_string.o scr_tab.o screen.o
OBJS+=scroll.o selcmp.o selection.o show_selection.o ttyinit.o
OBJS+=wm_del_win.o xevents.o xsetup.o xvt.o handle_sgr.o scroll_up.o
CFLAGS+=-D_XOPEN_SOURCE=700 --std=c99
CFLAGS+=-Wall -Wextra
$(exe): $(OBJS)
	$(CC) -o $(exe) $(OBJS) $(LIBS)
	ls -l $(exe) >> sz.log; tail sz.log
PREFIX=/usr
install:
	install -D $(exe) $(DESTDIR)$(PREFIX)/bin
clean:
	rm -f $(exe) $(OBJS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
