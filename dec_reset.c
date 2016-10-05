/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "dec_reset.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/util.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "sbar.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "screen.h"
#include "xsetup.h"

#include <stddef.h>

//#define DEBUG_RESET
#ifndef DEBUG_RESET
#undef LOG
#define LOG(...)
#endif

void dec_reset(struct Token * restrict token)
{
	LOG("handle_reset(%d)", token->arg[0]);
	const bool is_set = token->type == TK_SET;
	struct JBXVTPrivateModes * m = &jbxvt.mode;

	static bool allow_deccolm = true;

	if (likely(token->private == '?')) {
		switch (token->arg[0]) {
		case 1: // DECCKM: cursor key mode
			set_keys(is_set, true);
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
			jbxvt_reset();
			break;
		case 6 : // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			m->decom = is_set;
			jbxvt_move(SCR->margin.top, 0, 0);
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
		case 25: // DECTCEM -- hide cursor
			jbxvt_set_scroll(0);
			draw_cursor(); // clear
			jbxvt.mode.dectcem = is_set;
			draw_cursor(); // draw
			break;
		case 30: // toggle scrollbar -- per rxvt
			jbxvt_toggle_sbar();
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
				save_cursor();
			else
				restore_cursor();
			jbxvt_change_screen(is_set);
			break;
		case 2004: // bracketed paste mode
			LOG("bracketed paste mode");
			m->bpaste = is_set;
			break;
#ifdef DEBUG
		default:
			LOG("Unhandled: %d\n", token->arg[0]);
#endif//DEBUG
		}
	} else if (!token->private && token->arg[0] == 4)
		m->insert = is_set;
}


