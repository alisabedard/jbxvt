# Copyright 2017, Jeffrey E. Bedard
include config.mk
exe=jbxvt
PREFIX=/usr
AWK=/usr/bin/gawk
CFLAGS+=-DUSE_LIKELY
CFLAGS+=-D_XOPEN_SOURCE=700 --std=c11
CFLAGS+=-Wall -Wextra
ldflags+=-lxcb -lxcb-keysyms -lxcb-cursor
objs=jbxvt.o lookup_key.o paint.o change_selection.o cmdtok.o esc.o
objs+=cursor.o display.o repaint.o save_selection.o move.o size.o gc.o
objs+=sbar.o erase.o selex.o edit.o command.o selection.o dcs.o utf.o
objs+=selreq.o scr_reset.o string.o screen.o scroll.o selend.o mc.o
objs+=xevents.o window.o xvt.o sgr.o dec_reset.o show_selection.o
objs+=mouse.o double.o dsr.o font.o color.o tab.o rstyle.o tk_char.o
objs+=xcb_screen.o mode.o button_events.o request.o
extra+=color_index.h
${exe}: libjb ${objs}
	${CC} ${CFLAGS} -o ${exe} ${objs} ${static} ${ldflags}
	strip -o ${exe}.tmp ${exe}
	ls -l ${exe}.tmp >> sz.log
	rm -f ${exe}.tmp
	tail -n 5 sz.log
include depend.mk
cases.c: cases.txt cases.scm
	scheme < cases.scm
JBXVTRenderStyle.h: JBXVTRenderStyle.txt JBXVTRenderStyle.scm
	scheme < JBXVTRenderStyle.scm
color_index.h: color_index.txt color_index.scm
	scheme < color_index.scm
JBXVTTokenIndex.h: JBXVTTokenIndex.txt JBXVTTokenIndex.scm
	scheme < JBXVTTokenIndex.scm
sgr_cases.c: sgr_cases.txt sgr_cases.scm
	scheme < sgr_cases.scm
dec_reset_cases.c: dec_reset_cases.txt dec_reset_cases.awk
	awk -f dec_reset_cases.awk dec_reset_cases.txt > \
		dec_reset_cases.c
bindest=${DESTDIR}${PREFIX}/bin
docdest=${DESTDIR}${PREFIX}/share/man/man1
install: ${exe}
	install -d ${bindest}
	install ${exe} ${bindest}
	install -d ${docdest}
	install ${exe}.1 ${docdest}
	tic -xs jbxvt.terminfo
depend:
	cc -E -MM *.c > depend.mk
clean:
	cd libjb && make clean
	rm -f ${exe} *.o *.gcda *.gcno *.gcov libjb/*.gcda \
		libjb/*.gcno
distclean: clean
	rm -f config.mk gcov.log
	rm -f cases.c JBXVTRenderStyle.h color_index.h JBXVTTokenIndex.h\
		sgr_cases.c
check:
	tests/rgb
	tests/sgr
	tests/dec_reset
	tests/cursor
	tests/screen
	reset
gcov:
	gcov -b *.c > gcov.log
d: # DEBUG build
	CFLAGS='-DDEBUG -ggdb -O0 -Werror' make -j8
f: # Optimized build
	${MAKE} clean
	CFLAGS='-Ofast -march=native -flto' make -j8
s: # Tiny build
	${MAKE} clean
	CFLAGS='-Os -march=native -flto' make -j8
cppcheck:
	cppcheck --enable=all --inconclusive --std=c11 \
		-D TIOCSCTTY -I /usr/include \
		. 2> cppcheck.log
	echo 'Results written to cppcheck.log'
libjb: libjb/libjb.a
libjb/libjb.a:
	cd libjb && ${MAKE} CC=${CC} libjb.a
#EOF
# DO NOT DELETE
