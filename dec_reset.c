/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "dec_reset.h"
#include "cursor.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "lookup_key.h"
#include "mode.h"
#include "sbar.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "screen.h"
#include <string.h>
#define DEBUG_RESET
#ifndef DEBUG_RESET
#undef LOG
#define LOG(...)
#endif
void jbxvt_dec_reset(xcb_connection_t * xc, struct JBXVTToken * restrict token)
{
	LOG("handle_reset(%d)", token->arg[0]);
	const bool is_set = token->type == JBXVT_TOKEN_SET;
	struct JBXVTPrivateModes * m = jbxvt_get_modes();
	if (JB_LIKELY(token->private == '?')) {
		static bool allow_deccolm = true;
		switch (token->arg[0]) {
		case 1: // DECCKM: cursor key mode
			jbxvt_set_keys(is_set, true);
			break;
		case 2: // DECANM: VT52/ANSI mode
			m->decanm = is_set;
			break;
		case 3: // DECCOLM: 80/132 col mode switch
			if (allow_deccolm)
				m->deccolm = is_set;
			break;
		case 4: // DECSCLM: slow scroll mode
			m->decsclm = is_set;
			break;
		case 5: // DECSCNM: set reverse-video mode
			m->decscnm = is_set;
			jbxvt_reset(xc);
			break;
		case 6 : // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			m->decom = is_set;
			jbxvt_move(xc, jbxvt_get_screen()->margin.top, 0, 0);
			break;
		case 7: // DECAWM
		case 45: // reverse wrap-around mode?
			m->decawm = is_set;
			break;
		case 9:
			LOG("X10 mouse");
			m->mouse_x10 = is_set;
			break;
		case 12:
			LOG("att610 (re)set blinking cursor");
			m->att610 = is_set;
			break;
		case 18:
			LOG("DECPFF");
			m->decpff = is_set;
			break;
		case 20: // line feed / new line mode
			LOG("LNM");
			m->lnm = is_set;
			break;
		case 25: // DECTCEM -- hide cursor
			jbxvt_set_scroll(xc, 0);
			jbxvt_draw_cursor(xc); // clear
			jbxvt_get_modes()->dectcem = is_set;
			jbxvt_draw_cursor(xc); // draw
			break;
		case 30: // toggle scrollbar -- per rxvt
			jbxvt_toggle_scrollbar(xc);
			break;
		case 40: // allow deccolm
			allow_deccolm = is_set;
			break;
		case 1000:
			LOG("vt200 mouse");
			m->mouse_vt200 = is_set;
			break;
		case 1001:
			LOG("VT200 highlight mode");
			m->mouse_vt200hl = is_set;
			break;
		case 1002:
			LOG("button event mouse");
			m->mouse_btn_evt = is_set;
			break;
		case 1003:
			LOG("any event mouse");
			m->mouse_any_evt = is_set;
			break;
		case 1004:
			LOG("focus event mouse");
			m->mouse_focus_evt = is_set;
			break;
		case 1005:
			LOG("UTF-8 ext mode mouse");
			m->mouse_ext = is_set;
			break;
		case 1006:
			LOG("sgr ext mode mouse");
			m->mouse_sgr = is_set;
			break;
		case 1007:
			LOG("alternate scroll");
			m->mouse_alt_scroll = is_set;
			break;
		case 1015:
			LOG("urxvt ext mode mouse");
			m->mouse_urxvt = is_set;
			break;
		case 47: // switch to main screen
		case 1047:
		case 1048:
		case 1049: // cursor restore and screen change
			if (is_set)
				jbxvt_save_cursor();
			else
				jbxvt_restore_cursor(xc);
			jbxvt_change_screen(xc, is_set);
			break;
		case 2004: // bracketed paste mode
			LOG("bracketed paste mode");
			m->bpaste = is_set;
			break;
		default:
			LOG("Unhandled: %d\n", token->arg[0]);
			break;
		}
	} else
		switch (token->arg[0]) {
		case 2:
			LOG("FIXME AM: keyboard action mode");
			break;
		case 4: // IRM: insert/replace mode
			m->insert = is_set;
			break;
		case 12:
			LOG("SRM: send/receive mode");
			break;
		case 20:
			LOG("LNM linefeed/newline mode");
			break;
		}
}
