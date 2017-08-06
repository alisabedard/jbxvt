/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#undef DEBUG
#include "dec_reset.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTToken.h"
#include "cursor.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "lookup_key.h"
#include "mode.h"
#include "move.h"
#include "sbar.h"
#include "scr_reset.h"
#include "screen.h"
static void dectcem(xcb_connection_t * xc, const bool is_set)
{
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc); // clear
	jbxvt_get_modes()->dectcem = is_set;
	jbxvt_draw_cursor(xc); // draw
} static void change_screen(xcb_connection_t * xc, const bool is_set)
{
	if (is_set)
		jbxvt_save_cursor();
	else
		jbxvt_restore_cursor(xc);
	jbxvt_change_screen(xc, is_set);
}
void jbxvt_dec_reset(xcb_connection_t * xc,
	struct JBXVTToken * restrict token)
{
	LOG("handle_reset(%d)", token->arg[0]);
	const bool is_set = token->type == JBXVT_TOKEN_SET;
	struct JBXVTPrivateModes * m = jbxvt_get_modes();
	if (JB_LIKELY(token->private == '?')) {
		switch (token->arg[0]) {
#include "dec_reset_cases.c"
		case 1: // DECCKM: cursor key mode
			jbxvt_set_keys(is_set, true);
			break;
		case 5: // DECSCNM: set reverse-video mode
			m->decscnm = is_set;
			jbxvt_reset(xc);
			break;
		case 6: // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			m->decom = is_set;
			jbxvt_move(xc, jbxvt_get_margin()->top, 0, 0);
			break;
		case 25: // DECTCEM -- hide cursor
			dectcem(xc, is_set);
			break;
		case 30: // toggle scrollbar -- per rxvt
			jbxvt_toggle_scrollbar(xc);
			break;
		case 47: // switch to main screen
		case 1047:
		case 1048:
		case 1049: // cursor restore and screen change
			change_screen(xc, is_set);
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
