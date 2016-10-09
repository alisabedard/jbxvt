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
#include "window.h"

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
	if (likely(token->private == '?')) {
		static bool allow_deccolm = true;
		switch (token->arg[0]) {
		case 1: // DECCKM: cursor key mode
			set_keys(is_set, true);
			break;
#define MODE(field) jbxvt.mode.field = is_set
		case 2: // DECANM: VT52/ANSI mode
			MODE(decanm);
			break;
		case 3: // DECCOLM: 80/132 col mode switch
			if (allow_deccolm)
				MODE(deccolm);
			break;
		case 4: // DECSCLM: slow scroll mode
			MODE(decsclm);
			break;
		case 5: // DECSCNM: set reverse-video mode
			MODE(decscnm);
			jbxvt_reset();
			break;
		case 6 : // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			MODE(decom);
			jbxvt_move(jbxvt.scr.current->margin.top, 0, 0);
			break;
		case 7: // DECAWM
		case 45: // reverse wrap-around mode?
			MODE(decawm);
			break;
		case 9:
			LOG("X10 mouse");
			MODE(mouse_x10);
			break;
		case 12:
			LOG("att610 (re)set blinking cursor");
			MODE(att610);
			break;
		case 18:
			LOG("DECPFF");
			MODE(decpff);
			break;
		case 25: // DECTCEM -- hide cursor
			jbxvt_set_scroll(0);
			draw_cursor(); // clear
			jbxvt.mode.dectcem = is_set;
			draw_cursor(); // draw
			break;
		case 30: // toggle scrollbar -- per rxvt
			jbxvt_toggle_scrollbar();
			break;
		case 40: // allow deccolm
			allow_deccolm = is_set;
			break;
		case 1000:
			LOG("vt200 mouse");
			MODE(mouse_vt200);
			break;
		case 1001:
			LOG("VT200 highlight mode");
			MODE(mouse_vt200hl);
			break;
		case 1002:
			LOG("button event mouse");
			MODE(mouse_btn_evt);
			break;
		case 1003:
			LOG("any event mouse");
			MODE(mouse_any_evt);
			break;
		case 1004:
			LOG("focus event mouse");
			MODE(mouse_focus_evt);
			break;
		case 1005:
			LOG("UTF-8 ext mode mouse");
			MODE(mouse_ext);
			break;
		case 1006:
			LOG("sgr ext mode mouse");
			MODE(mouse_sgr);
			break;
		case 1007:
			LOG("alternate scroll");
			MODE(mouse_alt_scroll);
			break;
		case 1015:
			LOG("urxvt ext mode mouse");
			MODE(mouse_urxvt);
			break;
		case 47: // switch to main screen
		case 1047:
		case 1048:
		case 1049: // cursor restore and screen change
			(is_set ? save_cursor : restore_cursor)();
			jbxvt_change_screen(is_set);
			break;
		case 2004: // bracketed paste mode
			LOG("bracketed paste mode");
			MODE(bpaste);
			break;
#ifdef DEBUG
		default:
			LOG("Unhandled: %d\n", token->arg[0]);
#endif//DEBUG
		}
	} else if (!token->private && token->arg[0] == 4)
		MODE(insert);
}


