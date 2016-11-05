change_selection.o: change_selection.c change_selection.h libjb/size.h \
 config.h cursor.h font.h screen.h JBXVTScreen.h selend.h selection.h \
 JBXVTSelectionUnit.h size.h window.h
cmdtok.o: cmdtok.c cmdtok.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cursor.h dcs.h esc.h libjb/log.h libjb/xcb.h lookup_key.h \
 mode.h JBXVTPrivateModes.h sbar.h scr_reset.h window.h xevents.h
color.o: color.c color.h libjb/xcb.h libjb/util.h paint.h libjb/size.h \
 xcb_screen.h window.h
command.o: command.c command.h libjb/size.h libjb/util.h cmdtok.h Token.h \
 TokenType.h libjb/file.h libjb/log.h size.h xevents.h
cursor.o: cursor.c cursor.h config.h font.h libjb/size.h libjb/log.h \
 mode.h JBXVTPrivateModes.h repaint.h rstyle.h sbar.h screen.h \
 JBXVTScreen.h size.h window.h
dcs.o: dcs.c dcs.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cmdtok.h libjb/log.h
dec_reset.o: dec_reset.c dec_reset.h Token.h TokenType.h cursor.h \
 libjb/log.h libjb/util.h lookup_key.h mode.h JBXVTPrivateModes.h sbar.h \
 scr_move.h scr_reset.h screen.h JBXVTScreen.h config.h libjb/size.h
display.o: display.c display.h JBXVTOptions.h color.h libjb/xcb.h font.h \
 libjb/size.h config.h cursor.h paint.h sbar.h screen.h JBXVTScreen.h \
 size.h xcb_screen.h window.h
double.o: double.c double.h cursor.h repaint.h screen.h JBXVTScreen.h \
 config.h libjb/size.h
dsr.o: dsr.c dsr.h cmdtok.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h libjb/log.h screen.h JBXVTScreen.h config.h
edit.o: edit.c edit.h cursor.h font.h libjb/size.h libjb/log.h \
 libjb/util.h paint.h libjb/xcb.h sbar.h window.h screen.h JBXVTScreen.h \
 config.h selection.h JBXVTSelectionUnit.h size.h
esc.o: esc.c esc.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cmdtok.h dcs.h mode.h JBXVTPrivateModes.h screen.h \
 JBXVTScreen.h config.h
font.o: font.c font.h libjb/size.h config.h cursor.h libjb/util.h \
 libjb/xcb.h
handle_sgr.o: handle_sgr.c handle_sgr.h Token.h TokenType.h color.h \
 libjb/xcb.h libjb/log.h libjb/util.h paint.h libjb/size.h rstyle.h
jbxvt.o: jbxvt.c command.h libjb/size.h libjb/util.h config.h cursor.h \
 display.h JBXVTOptions.h color.h libjb/xcb.h font.h sbar.h scroll.h \
 JBXVTSavedLine.h tab.h window.h xevents.h xvt.h
lookup_key.o: lookup_key.c lookup_key.h command.h libjb/size.h \
 libjb/util.h libjb/log.h sbar.h
mode.o: mode.c mode.h JBXVTPrivateModes.h
mouse.o: mouse.c mouse.h libjb/size.h cmdtok.h Token.h TokenType.h \
 command.h libjb/util.h libjb/log.h mode.h JBXVTPrivateModes.h size.h
paint.o: paint.c paint.h libjb/size.h libjb/xcb.h color.h color_index.h \
 font.h double.h libjb/log.h libjb/util.h rstyle.h xcb_screen.h window.h
repaint.o: repaint.c repaint.h font.h libjb/size.h paint.h libjb/xcb.h \
 sbar.h screen.h JBXVTScreen.h config.h scroll.h JBXVTSavedLine.h \
 show_selection.h size.h window.h
rstyle.o: rstyle.c rstyle.h
save_selection.o: save_selection.c save_selection.h selection.h \
 JBXVTSelectionUnit.h libjb/size.h config.h libjb/log.h screen.h \
 JBXVTScreen.h selend.h size.h
sbar.o: sbar.c sbar.h config.h cursor.h libjb/util.h paint.h libjb/size.h \
 libjb/xcb.h repaint.h scroll.h JBXVTSavedLine.h size.h window.h
scr_erase.o: scr_erase.c scr_erase.h config.h cursor.h font.h \
 libjb/size.h libjb/log.h sbar.h screen.h JBXVTScreen.h scroll.h \
 JBXVTSavedLine.h scr_reset.h selection.h JBXVTSelectionUnit.h size.h \
 window.h
scr_move.o: scr_move.c scr_move.h cursor.h libjb/log.h libjb/util.h \
 mode.h JBXVTPrivateModes.h sbar.h screen.h JBXVTScreen.h config.h \
 libjb/size.h selection.h JBXVTSelectionUnit.h size.h
scr_reset.o: scr_reset.c scr_reset.h color.h libjb/xcb.h command.h \
 libjb/size.h libjb/util.h config.h cursor.h libjb/log.h libjb/time.h \
 mode.h JBXVTPrivateModes.h paint.h repaint.h sbar.h scr_move.h screen.h \
 JBXVTScreen.h scroll.h JBXVTSavedLine.h size.h window.h
scr_string.o: scr_string.c scr_string.h config.h cursor.h font.h \
 libjb/size.h libjb/log.h libjb/time.h libjb/util.h mode.h \
 JBXVTPrivateModes.h paint.h libjb/xcb.h repaint.h rstyle.h sbar.h \
 screen.h JBXVTScreen.h scroll.h JBXVTSavedLine.h scr_move.h selection.h \
 JBXVTSelectionUnit.h size.h tab.h window.h
screen.o: screen.c screen.h JBXVTScreen.h config.h libjb/size.h cursor.h \
 libjb/log.h mode.h JBXVTPrivateModes.h repaint.h sbar.h scr_erase.h \
 scr_move.h scr_reset.h scroll.h JBXVTSavedLine.h size.h
scroll.o: scroll.c scroll.h JBXVTSavedLine.h config.h font.h libjb/size.h \
 libjb/log.h libjb/util.h paint.h libjb/xcb.h sbar.h screen.h \
 JBXVTScreen.h selection.h JBXVTSelectionUnit.h size.h window.h
selection.o: selection.c selection.h JBXVTSelectionUnit.h libjb/size.h \
 config.h libjb/log.h libjb/xcb.h libjb/util.h save_selection.h selend.h \
 screen.h JBXVTScreen.h show_selection.h size.h window.h
selend.o: selend.c selend.h libjb/size.h sbar.h screen.h JBXVTScreen.h \
 config.h selection.h JBXVTSelectionUnit.h size.h
selex.o: selex.c selex.h libjb/size.h change_selection.h screen.h \
 JBXVTScreen.h config.h selection.h JBXVTSelectionUnit.h selend.h size.h
selreq.o: selreq.c selreq.h command.h libjb/size.h libjb/util.h config.h \
 libjb/log.h selection.h JBXVTSelectionUnit.h window.h
show_selection.o: show_selection.c show_selection.h cursor.h font.h \
 libjb/size.h selection.h JBXVTSelectionUnit.h selend.h size.h window.h
size.o: size.c size.h libjb/size.h font.h libjb/util.h
tab.o: tab.c tab.h libjb/log.h screen.h JBXVTScreen.h config.h \
 libjb/size.h sbar.h size.h
tk_char.o: tk_char.c tk_char.h cmdtok.h Token.h TokenType.h command.h \
 libjb/size.h libjb/util.h libjb/log.h mode.h JBXVTPrivateModes.h \
 scr_move.h screen.h JBXVTScreen.h config.h scroll.h JBXVTSavedLine.h \
 tab.h
window.o: window.c window.h font.h libjb/size.h sbar.h scr_reset.h \
 screen.h JBXVTScreen.h config.h size.h
xcb_screen.o: xcb_screen.c xcb_screen.h libjb/xcb.h
xevents.o: xevents.c xevents.h cmdtok.h Token.h TokenType.h command.h \
 libjb/size.h libjb/util.h font.h libjb/log.h libjb/xcb.h lookup_key.h \
 mode.h JBXVTPrivateModes.h mouse.h sbar.h scr_move.h scr_reset.h \
 screen.h JBXVTScreen.h config.h selection.h JBXVTSelectionUnit.h selex.h \
 selreq.h window.h
xvt.o: xvt.c xvt.h cmdtok.h Token.h TokenType.h command.h libjb/size.h \
 libjb/util.h cursor.h dec_reset.h double.h dsr.h handle_sgr.h \
 libjb/log.h lookup_key.h mode.h JBXVTPrivateModes.h repaint.h sbar.h \
 edit.h scr_erase.h scr_move.h scr_reset.h scr_string.h screen.h \
 JBXVTScreen.h config.h scroll.h JBXVTSavedLine.h selreq.h size.h tab.h \
 tk_char.h window.h
