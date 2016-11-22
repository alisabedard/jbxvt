// Copyright 2016, Jeffrey E. Bedard
#include "tab.h"
#include "libjb/log.h"
#include "screen.h"
#include "sbar.h"
#include "size.h"
#include <string.h>
static bool tab_stops[JBXVT_MAX_COLUMNS];
// Set tab stops:
// -1 clears all, -2 sets default
void jbxvt_set_tab(int16_t i, const bool value)
{
	if (i == -1) // clear all
		memset(&tab_stops, 0, JBXVT_MAX_COLUMNS);
	else if (i == -2) // establish tab stop every 8 columns
		for (i = 0; i < JBXVT_MAX_COLUMNS; ++i)
			tab_stops[i] = (i % 8 == 0);
	else if (i >= 0) // assign
		tab_stops[i] = value;
}
//  Tab to the next tab_stop.
void jbxvt_tab(xcb_connection_t * xc)
{
	LOG("jbxvt_tab()");
	jbxvt_set_scroll(xc, 0);
	struct JBXVTScreen * restrict scr = jbxvt_get_current_screen();
	struct JBDim c = scr->cursor;
	{ // text scope
		uint8_t * restrict text = scr->text[c.y];
		text[c.x] = ' ';
		{ // cw scope
			const uint16_t cw = jbxvt_get_char_size().w;
			while (!tab_stops[++c.x] && c.x < cw)
				text[c.x] = ' ';
		}
	}
	scr->cursor.x = c.x;
}
void jbxvt_cht(xcb_connection_t * xc, int16_t v)
{
	while (v-- > 0)
		jbxvt_tab(xc);
}
