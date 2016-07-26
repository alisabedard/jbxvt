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

#include <stddef.h>

void dec_reset(Token * restrict token)
{
	LOG("handle_reset(%d)", token->arg[0]);
	const bool is_set = token->type == TK_SET;
	struct JBXVTScreenData * s = &jbxvt.scr;
	VTScreen * scr = s->current;
	struct JBXVTPrivateModes * m = &jbxvt.mode;
#define SET(i) s->s[0].i = is_set; s->s[1].i = is_set;
	if (likely(token->private == '?')) {
		switch (token->arg[0]) {
		case 1: // DECCKM
			set_keys(is_set, true);
			break;
		case 2:
			m->decanm = is_set;
			break;
		case 3: // DECCOLM: 80/132 col mode switch
			break;
		case 4: // DECSCLM: slow scroll mode
			m->decsclm = is_set;
			break;
		case 5: // DECSCNM: set reverse-video mode
			break;
		case 6 : // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			m->decom = is_set;
			scr_move(scr->margin.top, 0, 0);
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
			LOG("12: att610 stop blinking cursor");
			m->att610 = is_set;
			break;
		case 25: // DECTCEM -- hide cursor
			change_offset(0);
			cursor(CURSOR_DRAW); // clear
			jbxvt.mode.dectcem = is_set;
			cursor(CURSOR_DRAW); // draw
			break;
		case 30: // toggle scrollbar -- per rxvt
			switch_scrollbar();
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
			cursor(is_set ? CURSOR_SAVE : CURSOR_RESTORE);
			scr_change_screen(is_set);
			break;
#ifdef DEBUG
		default:
			LOG("Unhandled: %d\n", token->arg[0]);
#endif//DEBUG
		}
	} else if (!token->private && token->arg[0] == 4)
		  scr->insert = is_set;
}


