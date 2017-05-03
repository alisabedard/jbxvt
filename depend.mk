button_events.o: button_events.c button_events.h JBXVTSelectionUnit.h \
 libjb/JBDim.h command.h libjb/util.h font.h mouse.h move.h sbar.h \
 screen.h selection.h selex.h selreq.h window.h
cases.o: cases.c
change_selection.o: change_selection.c change_selection.h cursor.h font.h \
 libjb/JBDim.h libjb/log.h selend.h selection.h size.h window.h
cmdtok.o: cmdtok.c cmdtok.h JBXVTToken.h JBXVTTokenType.h command.h \
 libjb/util.h cursor.h dcs.h esc.h libjb/log.h libjb/xcb.h mode.h utf.h \
 xevents.h
color.o: color.c color.h libjb/xcb.h JBXVTOptions.h libjb/JBDim.h \
 libjb/macros.h paint.h xcb_screen.h window.h
command.o: command.c command.h libjb/util.h cmdtok.h libjb/file.h \
 libjb/JBDim.h libjb/log.h size.h
cursor.o: cursor.c cursor.h JBXVTPrivateModes.h JBXVTScreen.h JBXVTLine.h \
 config.h rstyle.h libjb/JBDim.h font.h libjb/macros.h mode.h repaint.h \
 sbar.h screen.h size.h window.h
dcs.o: dcs.c dcs.h JBXVTToken.h JBXVTTokenType.h cmdtok.h libjb/log.h
dec_reset.o: dec_reset.c dec_reset.h JBXVTPrivateModes.h JBXVTToken.h \
 JBXVTTokenType.h cursor.h libjb/JBDim.h libjb/log.h libjb/macros.h \
 lookup_key.h mode.h move.h sbar.h scr_reset.h screen.h
display.o: display.c display.h JBXVTOptions.h libjb/JBDim.h color.h \
 libjb/xcb.h cursor.h font.h paint.h window.h xcb_screen.h
double.o: double.c double.h JBXVTLine.h config.h rstyle.h cursor.h \
 repaint.h screen.h
dsr.o: dsr.c dsr.h command.h libjb/util.h cursor.h libjb/log.h mode.h
edit.o: edit.c edit.h JBXVTLine.h config.h rstyle.h cursor.h font.h \
 libjb/JBDim.h libjb/log.h libjb/macros.h paint.h screen.h selection.h \
 size.h window.h
erase.o: erase.c erase.h JBXVTLine.h config.h rstyle.h JBXVTScreen.h \
 libjb/JBDim.h cursor.h font.h libjb/log.h paint.h sbar.h move.h screen.h \
 scroll.h size.h string.h window.h
esc.o: esc.c esc.h JBXVTPrivateModes.h JBXVTToken.h JBXVTTokenType.h \
 cmdtok.h command.h libjb/util.h cursor.h dcs.h mode.h screen.h
font.o: font.c font.h JBXVTOptions.h libjb/JBDim.h libjb/util.h \
 libjb/xcb.h
jbxvt.o: jbxvt.c command.h libjb/util.h JBXVTOptions.h libjb/JBDim.h \
 config.h cursor.h display.h tab.h window.h xevents.h xvt.h
lookup_key.o: lookup_key.c lookup_key.h command.h libjb/util.h \
 libjb/log.h sbar.h
mc.o: mc.c mc.h JBXVTToken.h JBXVTTokenType.h command.h libjb/util.h \
 cursor.h libjb/log.h
mode.o: mode.c mode.h JBXVTPrivateModes.h JBXVTToken.h JBXVTTokenType.h \
 libjb/log.h
mouse.o: mouse.c mouse.h JBXVTPrivateModes.h command.h libjb/util.h \
 libjb/JBDim.h libjb/log.h mode.h size.h
move.o: move.c move.h JBXVTPrivateModes.h JBXVTScreen.h JBXVTLine.h \
 config.h rstyle.h libjb/JBDim.h cursor.h libjb/macros.h mode.h sbar.h \
 screen.h selection.h size.h
paint.o: paint.c paint.h libjb/JBDim.h color_index.h color.h libjb/xcb.h \
 double.h font.h libjb/log.h rstyle.h window.h xcb_screen.h
repaint.o: repaint.c repaint.h JBXVTScreen.h JBXVTLine.h config.h \
 rstyle.h libjb/JBDim.h font.h paint.h sbar.h screen.h scroll.h \
 show_selection.h size.h
request.o: request.c request.h JBXVTPrivateModes.h JBXVTToken.h \
 JBXVTTokenType.h command.h libjb/util.h cursor.h libjb/log.h mode.h
rstyle.o: rstyle.c rstyle.h
save_selection.o: save_selection.c save_selection.h JBXVTLine.h config.h \
 rstyle.h JBXVTSelectionData.h JBXVTSelectionUnit.h libjb/JBDim.h \
 screen.h selend.h size.h
sbar.o: sbar.c sbar.h config.h cursor.h libjb/JBDim.h libjb/macros.h \
 paint.h repaint.h scroll.h size.h window.h
scr_reset.o: scr_reset.c scr_reset.h JBXVTPrivateModes.h JBXVTScreen.h \
 JBXVTLine.h config.h rstyle.h libjb/JBDim.h color.h libjb/xcb.h \
 command.h libjb/util.h cursor.h libjb/log.h libjb/macros.h libjb/time.h \
 mode.h move.h repaint.h sbar.h screen.h size.h window.h
screen.o: screen.c screen.h JBXVTPrivateModes.h JBXVTScreen.h JBXVTLine.h \
 config.h rstyle.h libjb/JBDim.h cursor.h erase.h libjb/log.h mode.h \
 move.h repaint.h sbar.h scr_reset.h scroll.h size.h
scroll.o: scroll.c scroll.h JBXVTLine.h config.h rstyle.h JBXVTScreen.h \
 libjb/JBDim.h font.h libjb/log.h libjb/macros.h libjb/util.h paint.h \
 sbar.h screen.h selection.h size.h window.h
selection.o: selection.c selection.h JBXVTSelectionData.h \
 JBXVTSelectionUnit.h libjb/JBDim.h libjb/log.h libjb/macros.h \
 libjb/xcb.h save_selection.h selend.h show_selection.h size.h window.h
selend.o: selend.c selend.h JBXVTLine.h config.h rstyle.h \
 JBXVTSelectionUnit.h libjb/JBDim.h sbar.h screen.h selection.h size.h
selex.o: selex.c selex.h change_selection.h libjb/JBDim.h libjb/macros.h \
 selection.h selend.h size.h
selreq.o: selreq.c selreq.h command.h libjb/util.h config.h libjb/log.h \
 selection.h window.h
sgr.o: sgr.c sgr.h color.h libjb/xcb.h JBXVTToken.h JBXVTTokenType.h \
 libjb/log.h libjb/macros.h rstyle.h
show_selection.o: show_selection.c show_selection.h cursor.h font.h \
 libjb/JBDim.h selection.h selend.h size.h window.h
size.o: size.c size.h font.h libjb/JBDim.h
string.o: string.c string.h JBXVTLine.h config.h rstyle.h \
 JBXVTPrivateModes.h JBXVTScreen.h libjb/JBDim.h cursor.h font.h \
 libjb/log.h libjb/macros.h libjb/time.h mode.h paint.h sbar.h screen.h \
 scroll.h move.h selection.h size.h tab.h window.h
tab.o: tab.c tab.h JBXVTLine.h config.h rstyle.h JBXVTScreen.h \
 libjb/JBDim.h JBXVTToken.h JBXVTTokenType.h cursor.h libjb/log.h \
 screen.h sbar.h size.h
tk_char.o: tk_char.c tk_char.h JBXVTPrivateModes.h command.h libjb/util.h \
 cursor.h libjb/JBDim.h libjb/log.h mode.h move.h screen.h scroll.h tab.h
utf.o: utf.c utf.h JBXVTToken.h JBXVTTokenType.h cmdtok.h libjb/log.h
window.o: window.c window.h JBXVTOptions.h libjb/JBDim.h JBXVTToken.h \
 JBXVTTokenType.h color.h libjb/xcb.h config.h font.h libjb/util.h sbar.h \
 scr_reset.h size.h
xcb_screen.o: xcb_screen.c xcb_screen.h libjb/xcb.h
xevents.o: xevents.c xevents.h JBXVTPrivateModes.h button_events.h \
 command.h libjb/util.h libjb/JBDim.h libjb/log.h libjb/xcb.h \
 lookup_key.h mode.h mouse.h sbar.h scr_reset.h selection.h selex.h \
 selreq.h window.h
xvt.o: xvt.c xvt.h JBXVTPrivateModes.h JBXVTScreen.h JBXVTLine.h config.h \
 rstyle.h libjb/JBDim.h JBXVTToken.h JBXVTTokenType.h cases.h cmdtok.h \
 command.h libjb/util.h cursor.h double.h dsr.h edit.h erase.h \
 libjb/log.h lookup_key.h mc.h mode.h move.h request.h sbar.h scr_reset.h \
 screen.h scroll.h selreq.h size.h string.h tab.h tk_char.h window.h \
 cases.c
