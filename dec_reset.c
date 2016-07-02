#include "dec_reset.h"

#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "scr_move.h"
#include "token.h"
#include "xsetup.h"

void dec_reset(Token * restrict token)
{
	LOG("handle_reset(%d)", token->tk_arg[0]);

	const bool set = token->tk_type == TK_SET;
	VTScreen * scr = jbxvt.scr.current;

	if (likely(token->tk_private == '?')) {
		switch (token->tk_arg[0]) {
		case 1 :
			set_keys(set, true);
			break;
		case 2:
			scr->decanm = set;
			break;
		case 6 : // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			scr->decom = set;
			scr_move(0, 0, 0);
			break;
		case 7: // DECAWM
		case 45: // reverse wrap-around mode?
			scr->decawm = set;
			break;
		case 9:
			LOG("9: Client wants mouse X and Y");
			break;
		case 12: // att610 -- stop blinking cursor
			// N/A
			break;
		case 25: // DECTCEM -- hide cursor
			home_screen();
			cursor(CURSOR_DRAW); // clear
			scr->dectcem = set;
			cursor(CURSOR_DRAW); // draw
			break;
		case 30: // toggle scrollbar -- per rxvt
			switch_scrollbar();
			break;
		case 1000:
			LOG("Send mouse X and Y on button press and"
				" release");
			jbxvt.scr.s1.ptr_xy = set;
			jbxvt.scr.s2.ptr_xy = set;
			break;
		case 1002:
			LOG("Use cell motion mouse tracking");
			jbxvt.scr.s1.ptr_cell = set;
			jbxvt.scr.s2.ptr_cell = set;
			break;
		case 1003:
			LOG("Use all motion mouse tracking");
		case 1005:
			LOG("Enable UTF-8 mouse mode");
			break;
		case 1006:
			LOG("Enable SGR mouse mode");
			break;
		case 1015:
			LOG("Enable urxvt mouse mode");
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
			LOG("Unhandled: %d\n", token->tk_arg[0]);
#endif//DEBUG
		}
	} else if (token->tk_private == 0) {
		switch (token->tk_arg[0]) {
		case 4 :
			scr->insert = set;
			break;
#ifdef DEBUG
		default:
			LOG("Unhandled: %d\n", token->tk_arg[0]);
#endif//DEBUG
		}
	}
}


