change_selection.o: change_selection.c change_selection.h libjb/size.h \
 config.h cursor.h font.h jbxvt.h libjb/util.h selection.h \
 JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h JBXVTEvent.h \
 JBXVTScreen.h screen.h selend.h window.h
cmdtok.o: cmdtok.c cmdtok.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cursor.h dcs.h esc.h jbxvt.h selection.h \
 JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h config.h \
 JBXVTEvent.h JBXVTScreen.h libjb/log.h libjb/xcb.h lookup_key.h sbar.h \
 scr_reset.h window.h xevents.h
color.o: color.c color.h libjb/xcb.h jbxvt.h libjb/util.h selection.h \
 JBXVTSelectionUnit.h libjb/size.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h paint.h screen.h window.h
command.o: command.c command.h libjb/size.h libjb/util.h cmdtok.h Token.h \
 TokenType.h jbxvt.h selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h \
 JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h libjb/file.h \
 libjb/log.h xevents.h
cursor.o: cursor.c cursor.h config.h font.h libjb/size.h jbxvt.h \
 libjb/util.h selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h \
 JBXVTSavedLine.h JBXVTEvent.h JBXVTScreen.h libjb/log.h repaint.h \
 rstyle.h sbar.h screen.h window.h
dcs.o: dcs.c dcs.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cmdtok.h jbxvt.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h
dec_reset.o: dec_reset.c dec_reset.h Token.h TokenType.h cursor.h jbxvt.h \
 libjb/util.h selection.h JBXVTSelectionUnit.h libjb/size.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h lookup_key.h sbar.h scr_move.h scr_reset.h screen.h
display.o: display.c display.h JBXVTOptions.h color.h libjb/xcb.h font.h \
 libjb/size.h cursor.h jbxvt.h libjb/util.h selection.h \
 JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h config.h \
 JBXVTEvent.h JBXVTScreen.h paint.h sbar.h screen.h window.h
double.o: double.c double.h cursor.h jbxvt.h libjb/util.h selection.h \
 JBXVTSelectionUnit.h libjb/size.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h repaint.h
dsr.o: dsr.c dsr.h cmdtok.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h jbxvt.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h
edit.o: edit.c edit.h cursor.h font.h libjb/size.h jbxvt.h libjb/util.h \
 selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h libjb/log.h paint.h libjb/xcb.h \
 sbar.h window.h screen.h
esc.o: esc.c esc.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cmdtok.h dcs.h jbxvt.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 screen.h
font.o: font.c font.h libjb/size.h config.h cursor.h jbxvt.h libjb/util.h \
 selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 JBXVTEvent.h JBXVTScreen.h libjb/xcb.h
handle_sgr.o: handle_sgr.c handle_sgr.h Token.h TokenType.h color.h \
 libjb/xcb.h jbxvt.h libjb/util.h selection.h JBXVTSelectionUnit.h \
 libjb/size.h JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h \
 JBXVTScreen.h libjb/log.h paint.h rstyle.h
jbxvt.o: jbxvt.c jbxvt.h libjb/util.h selection.h JBXVTSelectionUnit.h \
 libjb/size.h JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h \
 JBXVTScreen.h command.h cursor.h display.h JBXVTOptions.h color.h \
 libjb/xcb.h font.h sbar.h tab.h window.h xevents.h xvt.h
lookup_key.o: lookup_key.c lookup_key.h command.h libjb/size.h \
 libjb/util.h jbxvt.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h sbar.h
mouse.o: mouse.c mouse.h libjb/size.h cmdtok.h Token.h TokenType.h \
 command.h libjb/util.h jbxvt.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h screen.h
paint.o: paint.c paint.h libjb/size.h libjb/xcb.h color.h color_index.h \
 font.h double.h jbxvt.h libjb/util.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h rstyle.h screen.h window.h
repaint.o: repaint.c repaint.h font.h libjb/size.h jbxvt.h libjb/util.h \
 selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h paint.h libjb/xcb.h sbar.h \
 show_selection.h window.h
rstyle.o: rstyle.c rstyle.h
save_selection.o: save_selection.c save_selection.h selection.h \
 JBXVTSelectionUnit.h libjb/size.h config.h jbxvt.h libjb/util.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h selend.h
sbar.o: sbar.c sbar.h config.h cursor.h jbxvt.h libjb/util.h selection.h \
 JBXVTSelectionUnit.h libjb/size.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 JBXVTEvent.h JBXVTScreen.h paint.h libjb/xcb.h repaint.h scroll.h \
 window.h
scr_erase.o: scr_erase.c scr_erase.h config.h cursor.h font.h \
 libjb/size.h jbxvt.h libjb/util.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h sbar.h screen.h scroll.h scr_reset.h window.h
scr_move.o: scr_move.c scr_move.h cursor.h jbxvt.h libjb/util.h \
 selection.h JBXVTSelectionUnit.h libjb/size.h JBXVTPrivateModes.h \
 JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h libjb/log.h sbar.h
scr_reset.o: scr_reset.c scr_reset.h color.h libjb/xcb.h command.h \
 libjb/size.h libjb/util.h config.h cursor.h jbxvt.h selection.h \
 JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h JBXVTEvent.h \
 JBXVTScreen.h libjb/log.h libjb/time.h paint.h repaint.h sbar.h \
 scr_move.h screen.h scroll.h window.h
scr_string.o: scr_string.c scr_string.h config.h cursor.h font.h \
 libjb/size.h jbxvt.h libjb/util.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h JBXVTEvent.h JBXVTScreen.h \
 libjb/log.h libjb/time.h paint.h libjb/xcb.h repaint.h rstyle.h sbar.h \
 screen.h scroll.h scr_move.h tab.h window.h
screen.o: screen.c screen.h libjb/size.h cursor.h font.h jbxvt.h \
 libjb/util.h selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h \
 JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h libjb/log.h \
 libjb/xcb.h repaint.h sbar.h scr_erase.h scr_move.h scr_reset.h scroll.h
scroll.o: scroll.c scroll.h font.h libjb/size.h jbxvt.h libjb/util.h \
 selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h libjb/log.h paint.h libjb/xcb.h \
 sbar.h window.h
selection.o: selection.c selection.h JBXVTSelectionUnit.h libjb/size.h \
 config.h jbxvt.h libjb/util.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 JBXVTEvent.h JBXVTScreen.h libjb/log.h libjb/xcb.h save_selection.h \
 selend.h screen.h show_selection.h window.h
selend.o: selend.c selend.h libjb/size.h jbxvt.h libjb/util.h selection.h \
 JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h config.h \
 JBXVTEvent.h JBXVTScreen.h sbar.h
selex.o: selex.c selex.h libjb/size.h change_selection.h jbxvt.h \
 libjb/util.h selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h \
 JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h screen.h selend.h
selreq.o: selreq.c selreq.h command.h libjb/size.h libjb/util.h jbxvt.h \
 selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h libjb/log.h window.h
show_selection.o: show_selection.c show_selection.h cursor.h font.h \
 libjb/size.h jbxvt.h libjb/util.h selection.h JBXVTSelectionUnit.h \
 JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h JBXVTScreen.h \
 screen.h selend.h window.h
tab.o: tab.c tab.h jbxvt.h libjb/util.h selection.h JBXVTSelectionUnit.h \
 libjb/size.h JBXVTPrivateModes.h JBXVTSavedLine.h config.h JBXVTEvent.h \
 JBXVTScreen.h libjb/log.h sbar.h
window.o: window.c window.h font.h libjb/size.h jbxvt.h libjb/util.h \
 selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h sbar.h scr_reset.h screen.h
xevents.o: xevents.c xevents.h JBXVTEvent.h cmdtok.h Token.h TokenType.h \
 command.h libjb/size.h libjb/util.h font.h jbxvt.h selection.h \
 JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h config.h \
 JBXVTScreen.h libjb/log.h libjb/xcb.h mouse.h sbar.h scr_move.h selex.h \
 selreq.h window.h
xvt.o: xvt.c xvt.h cmdtok.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cursor.h dec_reset.h double.h dsr.h handle_sgr.h jbxvt.h \
 selection.h JBXVTSelectionUnit.h JBXVTPrivateModes.h JBXVTSavedLine.h \
 config.h JBXVTEvent.h JBXVTScreen.h libjb/log.h lookup_key.h repaint.h \
 sbar.h edit.h scr_erase.h scr_move.h scr_reset.h scr_string.h screen.h \
 scroll.h selreq.h tab.h window.h
