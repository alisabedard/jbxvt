CFLAGS=-Os -march=native
#CFLAGS+=-flto # doesn't work with clang
#CFLAGS=-ggdb
#CFLAGS+=-DDEBUG
#CC=clang

exe=jbxvt
LIBS=-lX11
OBJS=$(patsubst %.c,%.o,$(wildcard *.c))
CFLAGS+=-DLINUX -D_XOPEN_SOURCE=700 --std=c99
CFLAGS+=-Wall -Wextra -Werror
$(exe): $(OBJS)
	$(CC) -o $(exe) $(OBJS) $(LIBS)
	ls -l $(exe) >> sz.log; tail sz.log
PREFIX=/usr
install:
	install -D $(exe) $(DESTDIR)$(PREFIX)/bin
clean:
	rm -f $(exe) $(OBJS)
# DO NOT DELETE THIS LINE -- make depend depends on it.
