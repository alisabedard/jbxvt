// Copyright 2017, Jeffrey E. Bedard
#include "tk_char.h"
#include "JBXVTPrivateModes.h"
#include "command.h"
#include "cursor.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "mode.h"
#include "move.h"
#include "screen.h"
#include "scroll.h"
#include "string.h"
#include "tab.h"
static void form_feed(xcb_connection_t * restrict xc)
{
	const struct JBDim m = *jbxvt_get_margin();
	jbxvt_move(xc, 0, m.top, 0);
	if (jbxvt_get_modes()->decpff)
		dprintf(jbxvt_get_fd(), "FF");
	scroll(xc, m.top, m.bottom, m.bottom - m.top);
}
void jbxvt_handle_tk_char(xcb_connection_t * xc, const uint8_t tk_char)
{
	switch (tk_char) {
	case '\n': // handle line feed
	case 013: // vertical tab
		jbxvt_move(xc, 0, 1, JBXVT_ROW_RELATIVE);
		break;
	case '\f': // form feed
		form_feed(xc);
		break;
	case '\r': // handle carriage return
		jbxvt_move(xc, 0, 0, JBXVT_ROW_RELATIVE);
		break;
	case '\b': // handle a backspace
		jbxvt_move(xc, -1, 0, JBXVT_COLUMN_RELATIVE
			| JBXVT_ROW_RELATIVE);
		break;
	case '\t': // handle tab
		jbxvt_tab(xc, 1);
		break;
	case 005: // ENQ
		dprintf(jbxvt_get_fd(), "%s?6c", jbxvt_get_csi()); // VT102
		break;
	case '\016': // change to char set G1
		LOG("charset G1");
		jbxvt_get_modes()->charsel = 1;
		break;
	case '\017': // change to char set G0
		LOG("charset G0");
		jbxvt_get_modes()->charsel = 0;
		break;
	}
}
