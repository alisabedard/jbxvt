/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xvt.h"

#include "change_offset.h"
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "handle_sgr.h"
#include "log.h"
#include "sbar.h"
#include "screen.h"
#include "scr_delete_characters.h"
#include "scr_erase.h"
#include "scr_extend_selection.h"
#include "scr_insert_characters.h"
#include "scr_move.h"
#include "scr_refresh.h"
#include "scr_request_selection.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "scr_tab.h"
#include "selection.h"
#include "token.h"
#include "ttyinit.h"
#include "xsetup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool jbxvt_size_set;

static void handle_reset(struct tokenst * restrict token)
{
	LOG("handle_reset(%d)", token->tk_arg[0]);
	const bool set = token->tk_type == TK_SET;
	if (likely(token->tk_private == '?')) {
		switch (token->tk_arg[0]) {
		case 1 :
			set_keys(set, true);
			break;
		case 2:
			jbxvt.scr.current->decanm = set;
			break;
		case 1047:
			scr_change_screen(set);
			break;
		case 1048:
			cursor(set?CURSOR_SAVE:CURSOR_RESTORE);
			break;
		case 47: // switch to main screen
		case 1049: // cursor restore and screen change
			cursor(set?CURSOR_SAVE:CURSOR_RESTORE);
			scr_change_screen(set);
			break;
		case 6 : // DECOM normal cursor mode
			/* According to the spec, the cursor is reset to
			   the home position when this is changed.  */
			jbxvt.scr.current->decom = set;
			scr_move(0, 0, 0);
			break;
		case 7: // DECAWM
		case 45: // reverse wrap-around mode?
			jbxvt.scr.current->decawm = set;
			break;
		case 12: // att610 -- stop blinking cursor
			// N/A
			break;
		case 25: // DECTCEM -- hide cursor
			home_screen();
#if 0
			cursor(CURSOR_DRAW); // clear
			if (!set) {
				cursor(CURSOR_DRAW); // draw
			}
#endif
			break;
#ifdef DEBUG
		default:
			LOG("Unhandled: %d\n", token->tk_arg[0]);
#endif//DEBUG
		}
	} else if (token->tk_private == 0) {
		switch (token->tk_arg[0]) {
		case 4 :
			jbxvt.scr.current->insert = set;
			break;
#ifdef DEBUG
		default:
			LOG("Unhandled: %d\n", token->tk_arg[0]);
#endif//DEBUG
		}
	}
}

static void handle_txtpar(struct tokenst * restrict token)
{
	switch (token->tk_arg[0]) {
	case 0 :
		change_name(token->tk_string, false);
		change_name(token->tk_string, true);
		break;
	case 1 :
		change_name(token->tk_string, true);
		break;
	case 2 :
		change_name(token->tk_string, false);
		break;
	}

}

static void handle_tk_char(const uint8_t tk_char)
{
	switch (tk_char) {
	case '\n': // handle line feed
	case 013: // vertical tab
	case 014: // form feed
		scr_index();
		break;
	case '\r': // handle carriage return
		scr_move(0,0,ROW_RELATIVE);
		break;
	case '\b': // handle a backspace
		scr_move(-1,0,COL_RELATIVE|ROW_RELATIVE);
		break;
	case '\t': // handle tab
		scr_tab();
		break;
	case 005: // ENQ
		cprintf("");
		break;
	}
}

static void handle_tk_expose(struct tokenst * restrict t)
{
	LOG("handle_tk_expose()");
	switch (t->tk_region) {
	case SCREEN :
		if(jbxvt_size_set){
			cursor(CURSOR_DRAW); // clear
			scr_refresh((xcb_rectangle_t){.x = t->tk_arg[0],
				.y = t->tk_arg[1], .width = t->tk_arg[2],
				.height = t->tk_arg[3]});
			cursor(CURSOR_DRAW); // draw
		} else {
			/*  Force a full reset if an exposure event
			 *  arrives after a resize.  */
			scr_reset();
			jbxvt_size_set = true;
		}
		break;
	case SCROLLBAR :
		sbar_reset();
		break;
	}
}

void jbxvt_app_loop(void)
{
	LOG("app_loop");
	struct tokenst token;
	int32_t n; // sanitized first token
	int32_t * t; // shortcut to token.tk_arg
	struct screenst * scr = jbxvt.scr.current;
app_loop_head:
	get_token(&token);
	t = token.tk_arg;
	n = t[0];
	n = n ? n : 1; // n is sanitized for ops with optional nonzero args
	switch (token.tk_type) {
	case TK_STRING :
		LOG("TK_STRING");
		scr_string(token.tk_string, token.tk_length,
			token.tk_nlcount);
		break;
	case TK_CHAR :
		LOG("TK_CHAR");
		handle_tk_char(token.tk_char);
		break;
	case TK_EOF :
		LOG("TK_EOF");
		quit(0, NULL);
		break;
	case TK_ENTRY :	// keyboard focus changed
		cursor(t[0] ? CURSOR_ENTRY_IN : CURSOR_ENTRY_OUT);
		break;
	case TK_FOCUS :
		cursor(t[0] ? CURSOR_FOCUS_IN : CURSOR_FOCUS_OUT);
		break;
	case TK_EXPOSE: // window exposed
		handle_tk_expose(&token);
		break;
	case TK_RESIZE :
		jbxvt_size_set = false;
		resize_window();
		break;
	case TK_TXTPAR:	// change title or icon name
		handle_txtpar(&token);
		break;
	case TK_SBSWITCH :
		switch_scrollbar();
		break;
	case TK_SBGOTO :
	/*  Move the display so that line represented by scrollbar value
	    is at the top of the screen.  */
		change_offset((jbxvt.scr.chars.height + jbxvt.scr.sline.top)
			* (jbxvt.scr.pixels.height - t[0])
			/ jbxvt.scr.pixels.height - jbxvt.scr.chars.height);
		break;
	case TK_SBDOWN :
		t[0] = - t[0];
		// fall through
	case TK_SBUP :
		change_offset(jbxvt.scr.offset - t[0] / jbxvt.X.font_height);
		break;
	case TK_SELSTART :
		scr_start_selection((xcb_point_t){t[0], t[1]}, CHAR);
		break;
	case TK_SELEXTND :
		scr_extend_selection((xcb_point_t){t[0], t[1]}, false);
		break;
	case TK_SELDRAG :
		scr_extend_selection((xcb_point_t){t[0], t[1]}, true);
		break;
	case TK_SELWORD :
		scr_start_selection((xcb_point_t){t[0], t[1]}, WORD);
		break;
	case TK_SELLINE :
		scr_start_selection((xcb_point_t){t[0], t[1]}, LINE);
		break;
	case TK_SELECT :
		LOG("TK_SELECT");
		scr_make_selection(n);
		break;
	case TK_SELCLEAR :
		LOG("TK_SELCLEAR");
		scr_clear_selection();
		break;
	case TK_SELREQUEST :
		LOG("TK_SELREQUEST");
		scr_send_selection(t[0], t[1], t[2], t[3]);
		break;
	case TK_SELINSRT :
		LOG("TK_SELINSRT");
		scr_request_selection(t[0], t[1], t[2]);
		break;
	case TK_SELNOTIFY :
		LOG("TK_SELNOTIFY");
		// arg 0 is time, unused
		scr_paste_primary(t[1], t[2]);
		break;
	case TK_CUU: // cursor up
		LOG("TK_CUU");
		scr_move(0, -n, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUD: // cursor down
		LOG("TK_CUD");
		scr_move(0, n, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUF: // cursor forward
		LOG("TK_CUF");
		n = token.tk_arg[0];
		scr_move(n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUB: // cursor back
		LOG("TK_CUB");
		n = token.tk_arg[0];
		scr_move(-n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_HVP: // horizontal vertical position
		LOG("TK_HVP");
		// fall through
	case TK_CUP: // position cursor
		LOG("TK_CUP n: %d, 0: %d, 1: %d", token.tk_nargs,
			t[0], t[1]);
		switch(token.tk_nargs) {
		case 0:
			scr_move(0, 0, 0);
			break;
		case 1:
			scr_move(0, t[0] - 1, scr->decom ? ROW_RELATIVE : 0);
			break;
		case 2:
			scr_move(t[1] - 1, t[0] - 1, scr->decom
				? COL_RELATIVE | ROW_RELATIVE : 0);
			break;
		}
		break;
	case TK_ED :
		LOG("TK_ED"); // don't use n
		scr_erase_screen(t[0]);
		break;
	case TK_EL: // erase line
		LOG("TK_EL"); // don't use n
		scr_erase_line(t[0]);
		break;
	case TK_IL :
		LOG("TK_IL: %d", n);
		scr_insert_lines(n);
		break;
	case TK_DL :
		LOG("TK_DL");
		scr_delete_lines(n);
		break;
	case TK_DCH :
		LOG("TK_DCH");
		scr_delete_characters(n);
		break;
	case TK_ICH :
		LOG("TK_ICH");
		scr_insert_characters(n);
		break;
	case TK_SET :
		LOG("TK_SET");
		// fall through
	case TK_RESET :
		LOG("TK_RESET");
		handle_reset(&token);
		break;
	case TK_SGR :
		LOG("TK_SGR");
		handle_sgr(&token);
		break;
	case TK_DSR :		/* request for information */
		LOG("TK_DSR");
		switch (token.tk_arg[0]) {
		case 6 :
			cursor(CURSOR_REPORT);
			break;
		case 7 :
			//  Send the name of the display to the command.
			cprintf("%s\r", getenv("DISPLAY"));
			break;
		}
		break;
	case TK_DECSTBM: // set top and bottom margins.
		LOG("TK_DECSTBM");
		switch (token.tk_nargs) {
		case 0:
			scr->margin.top = 0;
			scr->margin.bottom = jbxvt.scr.chars.height - 1;
			break;
		case 1:
			scr->margin.top = 0;
			scr->margin.bottom = t[0] - 1;
			break;
		case 2:
			scr->margin.top = t[0] - 1;
			scr->margin.bottom = t[1] - 1;
			break;
		}
		scr_move(0, 0, 0);
		break;
	case TK_DECSC :
		LOG("TK_DECSC");
		cursor(CURSOR_SAVE);
		break;
	case TK_DECRC :
		LOG("TK_DECRC");
		cursor(CURSOR_RESTORE);
		break;
	case TK_DECPAM :
		LOG("TK_DECPAM");
		set_keys(true, false);
		break;
	case TK_DECPNM :
		LOG("TK_DECPNM");
		set_keys(false, false);
		break;
	case TK_IND :		/* Index (same as \n) */
		LOG("TK_IND");
		scr_index();
		break;
	case TK_RI :		/* Reverse index */
		LOG("TK_RI");
		scr_rindex();
		break;
	case TK_DECID :
	case TK_DA :
		LOG("TK_DECID");
		// VT420, 132 col, selective erase, ansi color
		cprintf("'\033[?64;1;6;22c'");
		break;
	case TK_DECSWH :		/* ESC # digit */
		LOG("TK_DECSWH");
		if (token.tk_arg[0] == '8') // DECALN
			  scr_efill();
		break;
#ifdef DEBUG
	case TK_HTS :
		LOG("TK_HTS");
		break;
	case TK_NEL :
		LOG("TK_NEL");
		break;
	case TK_SS2 :
		LOG("TK_SS2");
		break;
	case TK_SS3 :
		LOG("TK_SS3");
		break;
	case TK_TBC :
		LOG("TK_TBC");
		break;
#endif//DEBUG
	}
#ifdef TK_DEBUG
	show_token(&token);
#endif /* TK_DEBUG */
	goto app_loop_head;
}

