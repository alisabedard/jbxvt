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
#include "scroll.h"
#include "scr_edit.h"
#include "scr_erase.h"
#include "scr_extend_selection.h"
#include "scr_move.h"
#include "scr_refresh.h"
#include "scr_request_selection.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "selection.h"
#include "token.h"
#include "ttyinit.h"
#include "xsetup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool jbxvt_size_set;

static void handle_reset(Token * restrict token)
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

static void handle_txtpar(Token * restrict token)
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

static inline void set_charsel(const uint8_t i)
{
	jbxvt.scr.s1.charsel = i;
	jbxvt.scr.s2.charsel = i;
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
	case '\016': // change to char set G1
		LOG("charset G1");
		set_charsel(1);
		break;
	case '\017': // change to char set G0
		LOG("charset G0");
		set_charsel(0);
		break;
	}
}

static void handle_tk_expose(const uint8_t region, const int16_t * arg)
{
	if (region != SCROLLBAR) {
		if(jbxvt_size_set){
			cursor(CURSOR_DRAW); // clear
			scr_refresh((xcb_rectangle_t){.x = arg[0],
				.y = arg[1], .width = arg[2],
				.height = arg[3]});
			cursor(CURSOR_DRAW); // draw
		} else {
			/*  Force a full reset if an exposure event
			 *  arrives after a resize.  */
			scr_reset();
			jbxvt_size_set = true;
		}
	} else { // SCROLLBAR
		sbar_reset();
	}
}

static void set_cset(const enum CharacterSet cs, const uint8_t i)
{
	jbxvt.scr.s1.charset[i] = cs;
	jbxvt.scr.s2.charset[i] = cs;
}

static void select_charset(const char c, const uint8_t i)
{
	switch(c) {
	case 'A':
		LOG("United Kingdom");
		set_cset(CHARSET_GB, i);
		break;
	case 'B': // default
		LOG("ASCII");
		set_cset(CHARSET_ASCII, i);
		break;
	case '0':
		LOG("Special graphics");
		set_cset(CHARSET_SG0, i);
		break;
	case '1':
		LOG("Alt char ROM standard graphics");
		set_cset(CHARSET_SG1, i);
		break;
	case '2':
		LOG("Alt char ROM special graphics");
		set_cset(CHARSET_SG2, i);
		break;
	default: // reset
		LOG("Unknown character set");
		set_cset(CHARSET_ASCII, i);
	}

}

static void decstbm(Token * restrict token)
{
	int16_t * restrict t = token->tk_arg;
	LOG("TK_DECSTBM args: %d, 0: %d, 1: %d",
		(int)token->tk_nargs, t[0], t[1]);
	VTScreen * restrict scr = jbxvt.scr.current;
	if (token->tk_private == '?') {
		// Restore private modes
		// FIXME:  Unimplemented
		LOG("FIXME:  DECRESTOREPM");
		// At least set char set back to ASCII
		set_cset(CHARSET_ASCII, 0);
		set_charsel(0);
	} else if (token->tk_nargs < 2 || t[0] >= t[1]) {
		LOG("DECSTBM reset");
		// reset
		scr->margin.top = 0;
		scr->margin.bottom = jbxvt.scr.chars.height - 1;
	} else { // set
		LOG("DECSTBM set");
		LOG("m.b: %d, c.y: %d", scr->margin.bottom, scr->cursor.y);
		scr->margin.top = t[0] - 1;
		scr->margin.bottom = t[1] - 1;
	}

}

void jbxvt_app_loop(void)
{
	LOG("app_loop");
	Token token;
	int16_t n; // sanitized first token
	int16_t * t; // shortcut to token.tk_arg
	VTScreen * scr = jbxvt.scr.current;
app_loop_head:
	get_token(&token);
	t = token.tk_arg;
	// n is sanitized for ops with optional args
	n = t[0] ? t[0] : 1;
	switch (token.tk_type) {
	case TK_STRING :
		//LOG("TK_STRING");
		scr_string(token.tk_string, token.tk_length,
			token.tk_nlcount);
		break;
	case TK_CHAR :
		//LOG("TK_CHAR");
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
		//cprintf("\033[%c]", t[0] ? 'I' : 'O');
		cursor(t[0] ? CURSOR_FOCUS_IN : CURSOR_FOCUS_OUT);
		break;
	case TK_EXPOSE: // window exposed
		handle_tk_expose(token.tk_region, token.tk_arg);
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
	case TK_SD: // scroll down n lines
		LOG("TK_SD: %d", n);
		t[0] = - t[0];
		// fall through
	case TK_SU: // scroll up n lines;
		LOG("TK_SU");
		scroll(scr->margin.top, scr->margin.bot, t[0]);
		break;
	case TK_SBDOWN :
		t[0] = - t[0];
		// fall through
	case TK_SBUP :
		change_offset(jbxvt.scr.offset - t[0]
			/ jbxvt.X.font_size.height);
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
		request_selection();
		break;
	case TK_SELNOTIFY :
		LOG("TK_SELNOTIFY");
		// arg 0 is time, unused
		paste_primary(t[1], t[2]);
		break;
	case TK_CUU: // cursor up
		LOG("TK_CUU: args: %d, t[0]: %d, n: %d",
			token.tk_nargs, t[0], n);
		scr_move(0, -n, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUD: // cursor down
		LOG("TK_CUD: args: %d, t[0]: %d, n: %d",
			token.tk_nargs, t[0], n);
		scr_move(0, n, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUF: // cursor forward
		LOG("TK_CUF: args: %d, t[0]: %d, n: %d",
			token.tk_nargs, t[0], n);
		scr_move(n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUB: // cursor back
		LOG("TK_CUB: args: %d, t[0]: %d, n: %d",
			token.tk_nargs, t[0], n);
		scr_move(-n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_HVP: // horizontal vertical position
		LOG("TK_HVP");
		// fall through
	case TK_CUP: // position cursor
		LOG("TK_CUP");
		scr_move(t[1] - 1, t[0] - 1, scr->decom
			? ROW_RELATIVE | COL_RELATIVE : 0);
		break;
	case TK_VPA: // vertical position absolute
		LOG("TK_VPA");
		scr_move(0, t[0] - 1, COL_RELATIVE);
		break;
	case TK_VPR: // vertical position relative
		LOG("TK_VPR");
		scr_move(0, t[0] - 1, COL_RELATIVE|ROW_RELATIVE);
		break;
	case TK_CHA: // cursor CHaracter Absolute column
		LOG("TK_CHA");
		scr_move(t[0] - 1, 0, ROW_RELATIVE);
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
	case TK_ECH:
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
		//LOG("TK_SGR");
		handle_sgr(&token);
		break;
	case TK_DSR :		/* request for information */
		LOG("TK_DSR");
		switch (token.tk_arg[0]) {
		case 5: // command from host requesting status
			// 0 is response for 'Ready, no malfunctions'
			cprintf("0");
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
		decstbm(&token);
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
		cprintf("\033[?6c"); // VT102
		break;
	case TK_DECSWH :		/* ESC # digit */
		LOG("TK_DECSWH");
		if (token.tk_arg[0] == '8') // DECALN
			  scr_efill();
		break;
	case TK_NEL : // move to first position on next line down.
		scr_move(0, scr->cursor.y + 1, 0);
		LOG("TK_NEL: NExt Line");
		break;
	case TK_RIS: // Reset Initial State
		scr_reset();
		break;
	case TK_DECSAVEPM: // Save private modes
		LOG("TK_DECSAVEPM");
		// FIXME: Unimplemented
		// At least set char set back to ASCII
		// This fixes the links browser
		set_cset(CHARSET_ASCII, 0);
		set_charsel(0);
		break;
	case TK_SCS0: // DEC SCS G0
		LOG("TK_SCS0");
		select_charset(t[0], 0);
		break;
	case TK_SCS1: // DEC SCS G1
		LOG("TK_SCS1");
		select_charset(t[0], 1);
		break;
#ifdef DEBUG
	case TK_HTS :
		LOG("TK_HTS");
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
	default:
		if(token.tk_type) { // Ignore TK_NULL
			LOG("Unhandled token: %d", token.tk_type);
			exit(1);
		}
		break;
#endif//DEBUG
	}
	goto app_loop_head;
}

