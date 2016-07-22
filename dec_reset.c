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
#include "screen.h"
#include "xsetup.h"

void dec_reset(Token * restrict token)
{
	LOG("handle_reset(%d)", token->arg[0]);

	const bool set = token->type == TK_SET;
	VTScreen * scr = jbxvt.scr.current;
	VTScreen * s1 = &jbxvt.scr.s1;
	VTScreen * s2 = &jbxvt.scr.s2;
#undef SET
#define SET(i) {s1->i = set; s2->i = set;}

	if (likely(token->private == '?')) {
		switch (token->arg[0]) {
		case 1: // DECCKM
			set_keys(set, true);
			break;
		case 2:
			SET(decanm);
			break;
		case 3: // DECCOLM: 80/132 col mode switch
			break;
		case 5: // DECSCNM: set reverse-video mode
			break;
		case 6 : // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			SET(decom);
			scr_move(scr->margin.top, 0, 0);
			break;
		case 7: // DECAWM
		case 45: // reverse wrap-around mode?
			SET(decawm);
			break;
		case 9:
			LOG("X10 mouse");
			SET(mouse_x10);
			break;
		case 12:
			LOG("12: att610 stop blinking cursor");
			SET(att610);
			break;
		case 25: // DECTCEM -- hide cursor
			change_offset(0);
			cursor(CURSOR_DRAW); // clear
			SET(dectcem);
			cursor(CURSOR_DRAW); // draw
			break;
		case 30: // toggle scrollbar -- per rxvt
			switch_scrollbar();
			break;
		case 1000:
			LOG("vt200 mouse");
			SET(mouse_vt200);
			break;
		case 1001:
			LOG("VT200 highlight mode");
			SET(mouse_vt200hl);
			break;
		case 1002:
			LOG("button event mouse");
			SET(mouse_btn_evt);
			break;
		case 1003:
			LOG("any event mouse");
			SET(mouse_any_evt);
			break;
		case 1004:
			LOG("focus event mouse");
			SET(mouse_focus_evt);
			break;
		case 1005:
			LOG("UTF-8 ext mode mouse");
			SET(mouse_ext);
			break;
		case 1006:
			LOG("sgr ext mode mouse");
			SET(mouse_sgr);
			break;
		case 1007:
			LOG("alternate scroll");
			SET(mouse_alt_scroll);
			break;
		case 1015:
			LOG("urxvt ext mode mouse");
			SET(mouse_urxvt);
			break;
		case 47: // switch to main screen
		case 1047:
		case 1048:
		case 1049: // cursor restore and screen change
			cursor(set?CURSOR_SAVE:CURSOR_RESTORE);
			scr_change_screen(set);
			break;
#ifdef DEBUG
		default:
			LOG("Unhandled: %d\n", token->arg[0]);
#endif//DEBUG
		}
	} else if (!token->private && token->arg[0] == 4)
		  scr->insert = set;
}


