change_selection.o: change_selection.c change_selection.h libjb/JBDim.h \
 config.h cursor.h font.h libjb/log.h screen.h JBXVTScreen.h JBXVTLine.h \
 rstyle.h selend.h selection.h size.h window.h
cmdtok.o: cmdtok.c cmdtok.h command.h libjb/JBDim.h libjb/util.h cursor.h \
 dcs.h JBXVTToken.h JBXVTTokenType.h esc.h libjb/log.h libjb/xcb.h mode.h \
 JBXVTPrivateModes.h xevents.h
color.o: color.c color.h libjb/xcb.h libjb/macros.h paint.h libjb/JBDim.h \
 xcb_screen.h window.h
command.o: command.c command.h libjb/JBDim.h libjb/util.h cmdtok.h \
 libjb/file.h libjb/log.h size.h xevents.h
cursor.o: cursor.c cursor.h config.h font.h libjb/JBDim.h libjb/log.h \
 mode.h JBXVTPrivateModes.h repaint.h rstyle.h sbar.h screen.h \
 JBXVTScreen.h JBXVTLine.h size.h window.h
dcs.o: dcs.c dcs.h JBXVTToken.h JBXVTTokenType.h command.h libjb/JBDim.h \
 libjb/util.h cmdtok.h libjb/log.h
dec_reset.o: dec_reset.c dec_reset.h JBXVTToken.h JBXVTTokenType.h \
 cursor.h libjb/log.h libjb/macros.h lookup_key.h mode.h \
 JBXVTPrivateModes.h sbar.h scr_move.h scr_reset.h screen.h JBXVTScreen.h \
 JBXVTLine.h config.h rstyle.h libjb/JBDim.h
display.o: display.c display.h JBXVTOptions.h color.h libjb/xcb.h font.h \
 libjb/JBDim.h config.h cursor.h paint.h sbar.h size.h xcb_screen.h \
 window.h
double.o: double.c double.h cursor.h repaint.h screen.h JBXVTScreen.h \
 JBXVTLine.h config.h rstyle.h libjb/JBDim.h
dsr.o: dsr.c dsr.h cmdtok.h command.h libjb/JBDim.h libjb/util.h \
 libjb/log.h screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h
edit.o: edit.c edit.h cursor.h font.h libjb/JBDim.h libjb/log.h \
 libjb/macros.h paint.h screen.h JBXVTScreen.h JBXVTLine.h config.h \
 rstyle.h size.h window.h
erase.o: erase.c erase.h cursor.h font.h libjb/JBDim.h libjb/log.h sbar.h \
 screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h size.h window.h
esc.o: esc.c esc.h JBXVTToken.h JBXVTTokenType.h cmdtok.h command.h \
 libjb/JBDim.h libjb/util.h dcs.h mode.h JBXVTPrivateModes.h screen.h \
 JBXVTScreen.h JBXVTLine.h config.h rstyle.h
font.o: font.c font.h libjb/JBDim.h libjb/util.h libjb/xcb.h
jbxvt.o: jbxvt.c command.h libjb/JBDim.h libjb/util.h config.h cursor.h \
 display.h JBXVTOptions.h color.h libjb/xcb.h font.h tab.h window.h \
 xevents.h xvt.h
lookup_key.o: lookup_key.c lookup_key.h command.h libjb/JBDim.h \
 libjb/util.h libjb/log.h sbar.h
mode.o: mode.c mode.h JBXVTPrivateModes.h
mouse.o: mouse.c mouse.h libjb/JBDim.h cmdtok.h command.h libjb/util.h \
 libjb/log.h mode.h JBXVTPrivateModes.h size.h
paint.o: paint.c paint.h libjb/JBDim.h color_index.h color.h libjb/xcb.h \
 double.h font.h libjb/log.h rstyle.h window.h xcb_screen.h
repaint.o: repaint.c repaint.h font.h libjb/JBDim.h paint.h sbar.h \
 screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h scroll.h \
 show_selection.h size.h
rstyle.o: rstyle.c rstyle.h
save_selection.o: save_selection.c save_selection.h selection.h config.h \
 JBXVTSelectionData.h JBXVTSelectionUnit.h libjb/JBDim.h libjb/log.h \
 screen.h JBXVTScreen.h JBXVTLine.h rstyle.h selend.h size.h
sbar.o: sbar.c sbar.h config.h cursor.h libjb/macros.h paint.h \
 libjb/JBDim.h repaint.h scroll.h JBXVTLine.h rstyle.h size.h window.h
scr_move.o: scr_move.c scr_move.h cursor.h libjb/log.h libjb/macros.h \
 mode.h JBXVTPrivateModes.h sbar.h screen.h JBXVTScreen.h JBXVTLine.h \
 config.h rstyle.h libjb/JBDim.h selection.h size.h
scr_reset.o: scr_reset.c scr_reset.h color.h libjb/xcb.h command.h \
 libjb/JBDim.h libjb/util.h config.h cursor.h libjb/log.h libjb/macros.h \
 libjb/time.h mode.h JBXVTPrivateModes.h paint.h repaint.h sbar.h \
 scr_move.h screen.h JBXVTScreen.h JBXVTLine.h rstyle.h scroll.h size.h \
 window.h
screen.o: screen.c screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h \
 libjb/JBDim.h cursor.h libjb/log.h mode.h JBXVTPrivateModes.h repaint.h \
 sbar.h erase.h scr_move.h scr_reset.h scroll.h size.h
scroll.o: scroll.c scroll.h JBXVTLine.h config.h rstyle.h cursor.h font.h \
 libjb/JBDim.h libjb/log.h libjb/macros.h libjb/util.h paint.h sbar.h \
 screen.h JBXVTScreen.h selection.h size.h window.h
selection.o: selection.c selection.h JBXVTSelectionData.h \
 JBXVTSelectionUnit.h libjb/JBDim.h libjb/log.h libjb/macros.h \
 libjb/xcb.h save_selection.h selend.h show_selection.h size.h window.h
selend.o: selend.c selend.h libjb/JBDim.h JBXVTSelectionUnit.h sbar.h \
 screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h selection.h size.h
selex.o: selex.c selex.h libjb/JBDim.h change_selection.h libjb/macros.h \
 selection.h selend.h size.h
selreq.o: selreq.c selreq.h command.h libjb/JBDim.h libjb/util.h config.h \
 libjb/log.h selection.h window.h
sgr.o: sgr.c sgr.h color.h libjb/xcb.h JBXVTToken.h JBXVTTokenType.h \
 libjb/log.h libjb/macros.h rstyle.h
show_selection.o: show_selection.c show_selection.h cursor.h font.h \
 libjb/JBDim.h selection.h selend.h size.h window.h
size.o: size.c size.h libjb/JBDim.h font.h
string.o: string.c string.h config.h cursor.h font.h libjb/JBDim.h \
 libjb/log.h libjb/macros.h libjb/time.h libjb/util.h mode.h \
 JBXVTPrivateModes.h paint.h repaint.h rstyle.h sbar.h screen.h \
 JBXVTScreen.h JBXVTLine.h scroll.h scr_move.h selection.h size.h tab.h \
 window.h
tab.o: tab.c tab.h libjb/log.h screen.h JBXVTScreen.h JBXVTLine.h \
 config.h rstyle.h libjb/JBDim.h sbar.h size.h
tk_char.o: tk_char.c tk_char.h cmdtok.h command.h libjb/JBDim.h \
 libjb/util.h cursor.h libjb/log.h mode.h JBXVTPrivateModes.h scr_move.h \
 screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h scroll.h tab.h
window.o: window.c window.h font.h libjb/JBDim.h libjb/util.h libjb/xcb.h \
 sbar.h scr_reset.h screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h \
 size.h
xcb_screen.o: xcb_screen.c xcb_screen.h libjb/xcb.h
xevents.o: xevents.c xevents.h cmdtok.h command.h libjb/JBDim.h \
 libjb/util.h font.h JBXVTSelectionUnit.h libjb/log.h libjb/xcb.h \
 lookup_key.h mode.h JBXVTPrivateModes.h mouse.h sbar.h scr_move.h \
 scr_reset.h screen.h JBXVTScreen.h JBXVTLine.h config.h rstyle.h \
 selection.h selex.h selreq.h window.h
xvt.o: xvt.c xvt.h cmdtok.h command.h libjb/JBDim.h libjb/util.h cursor.h \
 dec_reset.h JBXVTToken.h JBXVTTokenType.h double.h dsr.h edit.h \
 libjb/log.h lookup_key.h mode.h JBXVTPrivateModes.h sbar.h erase.h \
 scr_move.h scr_reset.h sgr.h string.h screen.h JBXVTScreen.h JBXVTLine.h \
 config.h rstyle.h scroll.h selreq.h size.h tab.h tk_char.h window.h
