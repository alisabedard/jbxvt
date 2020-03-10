button_events.o: button_events.c button_events.h JBXVTCommandLimits.h JBXVTSelectionUnit.h font.h libjb/JBDim.h mouse.h move.h sbar.h screen.h selection.h selex.h selreq.h window.h
change_selection.o: change_selection.c change_selection.h font.h gc.h libjb/JBDim.h libjb/log.h selection.h selend.h size.h window.h
cmdtok.o: cmdtok.c cmdtok.h JBXVTCommandLimits.h JBXVTToken.h command.h cursor.h dcs.h esc.h libjb/log.h libjb/xcb.h utf.h xevents.h
color.o: color.c color.h JBXVTOptions.h gc.h libjb/macros.h window.h xcb_screen.h
command.o: command.c command.h cmdtok.h libjb/file.h libjb/JBDim.h libjb/log.h libjb/util.h size.h
cursor.o: cursor.c cursor.h JBXVTPrivateModes.h font.h gc.h libjb/macros.h mode.h repaint.h sbar.h size.h window.h
dcs.o: dcs.c dcs.h JBXVTToken.h cmdtok.h libjb/log.h
dec_reset.o: dec_reset.c dec_reset.h JBXVTPrivateModes.h JBXVTToken.h cursor.h libjb/JBDim.h libjb/log.h libjb/macros.h lookup_key.h mode.h move.h sbar.h scr_reset.h screen.h dec_reset_cases.c
dec_reset_cases.o: dec_reset_cases.c
display.o: display.c display.h paint.h xcb_screen.h JBXVTOptions.h color.h font.h gc.h window.h
double.o: double.c double.h cursor.h repaint.h
dsr.o: dsr.c dsr.h command.h cursor.h libjb/log.h mode.h
edit.o: edit.c edit.h cursor.h font.h gc.h libjb/log.h libjb/macros.h selection.h size.h window.h
erase.o: erase.c erase.h cursor.h font.h libjb/log.h sbar.h size.h window.h
esc.o: esc.c esc.h JBXVTPrivateModes.h JBXVTToken.h cmdtok.h command.h cursor.h dcs.h mode.h screen.h
font.o: font.c font.h JBXVTOptions.h libjb/JBDim.h libjb/util.h libjb/xcb.h xcb_id_getter.h
gc.o: gc.c gc.h xcb_id_getter.h
jbxvt.o: jbxvt.c command.h JBXVTOptions.h config.h cursor.h display.h tab.h window.h xvt.h
key_tables.o: key_tables.c
lookup_key.o: lookup_key.c lookup_key.h JBXVTCommandLimits.h JBXVTKeyMaps.h JBXVTKeySymType.h libjb/log.h libjb/util.h sbar.h key_tables.c
mc.o: mc.c mc.h JBXVTToken.h command.h cursor.h libjb/log.h
mode.o: mode.c mode.h JBXVTPrivateModes.h JBXVTToken.h libjb/log.h
mouse.o: mouse.c mouse.h JBXVTPrivateModes.h command.h libjb/JBDim.h libjb/log.h mode.h size.h
move.o: move.c move.h JBXVTPrivateModes.h JBXVTScreen.h cursor.h libjb/JBDim.h libjb/log.h libjb/macros.h mode.h sbar.h screen.h selection.h size.h
paint.o: paint.c paint.h JBXVTPaintContext.h JBXVTRenderStyle.h color.h color_index.h double.h font.h gc.h libjb/log.h window.h xcb_screen.h
repaint.o: repaint.c repaint.h JBXVTPaintContext.h JBXVTScreen.h config.h font.h gc.h libjb/log.h paint.h sbar.h screen.h scroll.h show_selection.h size.h window.h
request.o: request.c request.h JBXVTPrivateModes.h JBXVTToken.h command.h cursor.h libjb/log.h mode.h
rstyle.o: rstyle.c rstyle.h
save_selection.o: save_selection.c save_selection.h JBXVTLine.h JBXVTSelectionData.h screen.h selend.h size.h
sbar.o: sbar.c sbar.h config.h cursor.h gc.h libjb/JBDim.h libjb/macros.h repaint.h scroll.h size.h window.h xcb_id_getter.h
scr_reset.o: scr_reset.c scr_reset.h JBXVTPrivateModes.h JBXVTScreen.h color.h command.h config.h cursor.h libjb/log.h libjb/macros.h libjb/time.h mode.h move.h repaint.h sbar.h screen.h size.h window.h
screen.o: screen.c screen.h JBXVTPrivateModes.h cursor.h erase.h libjb/log.h mode.h move.h repaint.h sbar.h scr_reset.h scroll.h size.h
scroll.o: scroll.c scroll.h JBXVTLine.h JBXVTScreen.h font.h gc.h libjb/JBDim.h libjb/log.h libjb/macros.h libjb/util.h sbar.h screen.h selection.h size.h window.h
selection.o: selection.c selection.h JBXVTSelectionData.h libjb/log.h libjb/macros.h libjb/xcb.h save_selection.h selend.h show_selection.h size.h window.h
selend.o: selend.c selend.h JBXVTLine.h JBXVTSelectionUnit.h libjb/JBDim.h sbar.h screen.h selection.h size.h
selex.o: selex.c selex.h change_selection.h libjb/JBDim.h libjb/macros.h selection.h selend.h size.h
selreq.o: selreq.c selreq.h command.h config.h libjb/log.h libjb/util.h selection.h window.h
sgr.o: sgr.c sgr.h JBXVTRenderStyle.h JBXVTToken.h color.h libjb/log.h libjb/macros.h rstyle.h sgr_cases.c
sgr_cases.o: sgr_cases.c
show_selection.o: show_selection.c show_selection.h font.h gc.h libjb/JBDim.h selection.h selend.h size.h window.h
size.o: size.c size.h font.h libjb/JBDim.h
string.o: string.c string.h JBXVTPaintContext.h JBXVTPrivateModes.h cursor.h font.h gc.h libjb/log.h libjb/macros.h libjb/time.h mode.h paint.h sbar.h screen.h scroll.h selection.h size.h tab.h window.h
tab.o: tab.c tab.h JBXVTLine.h JBXVTScreen.h JBXVTToken.h config.h cursor.h libjb/JBDim.h libjb/log.h screen.h sbar.h size.h
tk_char.o: tk_char.c tk_char.h JBXVTPrivateModes.h command.h cursor.h libjb/JBDim.h libjb/log.h mode.h move.h screen.h scroll.h string.h tab.h
utf.o: utf.c utf.h JBXVTToken.h cmdtok.h libjb/log.h
window.o: window.c window.h JBXVTOptions.h JBXVTToken.h color.h config.h font.h libjb/util.h libjb/xcb.h sbar.h scr_reset.h size.h xcb_id_getter.h
xcb_screen.o: xcb_screen.c xcb_screen.h libjb/xcb.h
xevents.o: xevents.c xevents.h JBXVTPrivateModes.h button_events.h command.h libjb/JBDim.h libjb/log.h libjb/xcb.h lookup_key.h mode.h mouse.h sbar.h scr_reset.h selection.h selex.h selreq.h window.h
xvt.o: xvt.c xvt.h JBXVTPrivateModes.h JBXVTScreen.h JBXVTToken.h cases.h cmdtok.h command.h cursor.h double.h dsr.h edit.h erase.h libjb/JBDim.h libjb/log.h lookup_key.h mc.h mode.h move.h request.h sbar.h scr_reset.h screen.h scroll.h selreq.h size.h string.h tab.h tk_char.h window.h

