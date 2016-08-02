/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "xvt.h"

#include "cmdtok.h"
#include "cursor.h"
#include "dec_reset.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_edit.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "screen.h"
#include "scroll.h"
#include "selex.h"
#include "selreq.h"
#include "Token.h"
#include "xsetup.h"

#include <string.h>

static void handle_txtpar(Token * restrict token)
{
	switch (token->arg[0]) {
	case 0 :
		change_name(token->string, false);
		change_name(token->string, true);
		break;
	case 1 :
		change_name(token->string, true);
		break;
	case 2 :
		change_name(token->string, false);
		break;
	}
}

static void form_feed(void)
{
	const Size m = jbxvt.scr.current->margin;
	scr_move(0, m.top, 0);
	if (jbxvt.mode.decpff)
		cprintf("FF");
	scroll(m.top, m.bottom, m.bottom - m.top);
}

static void handle_tk_char(const uint8_t tk_char)
{
	VTScreen * s = jbxvt.scr.current;
	switch (tk_char) {
	case '\n': // handle line feed
		scr_index_from(1, s->margin.t);
		break;
	case 013: // vertical tab
		for (uint8_t i = s->cursor.y; i % 8; ++i)
			  scr_index_from(1, s->margin.t);
		break;
	case '\f': // form feed
		form_feed();
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
		jbxvt.mode.charsel = 1;
		break;
	case '\017': // change to char set G0
		LOG("charset G0");
		jbxvt.mode.charsel = 0;
		break;
	}
}

static void expose(const uint8_t region, const bool size_set)
{
	if (region == REGION_SCROLLBAR)
		sbar_reset();
	else if (size_set) {
		cursor(CURSOR_DRAW); // clear
		repaint();
		cursor(CURSOR_DRAW); // draw
	} else {
		/*  Force a full reset if an exposure event
		 *  arrives after a resize.  */
		scr_reset();
	}
}

static void select_charset(const char c, const uint8_t i)
{
	switch(c) {
	case 'A':
		LOG("United Kingdom");
		jbxvt.mode.charset[i] = CHARSET_GB;
		break;
	case 'B': // default
		LOG("ASCII");
		jbxvt.mode.charset[i] = CHARSET_ASCII;
		break;
	case '0':
		LOG("Special graphics");
		jbxvt.mode.charset[i] = CHARSET_SG0;
		break;
	case '1':
		LOG("Alt char ROM standard graphics");
		jbxvt.mode.charset[i] = CHARSET_SG1;
		break;
	case '2':
		LOG("Alt char ROM special graphics");
		jbxvt.mode.charset[i] = CHARSET_SG2;
		break;
	default: // reset
		LOG("Unknown character set");
		jbxvt.mode.charset[i] = CHARSET_ASCII;
	}
}

static void decstbm(Token * restrict token)
{
	int32_t * restrict t = token->arg;
	LOG("TK_DECSTBM args: %d, 0: %d, 1: %d",
		(int)token->nargs, t[0], t[1]);
	VTScreen * restrict s = jbxvt.scr.current;
	if (token->private == '?') { //DECRESTOREPM
		// Restore private modes
		memcpy(&jbxvt.mode, &jbxvt.saved_mode,
			sizeof(struct JBXVTPrivateModes));
	} else if (token->nargs < 2 || t[0] >= t[1]) {
		LOG("DECSTBM reset");
		// reset
		s->margin.top = 0;
		s->margin.bottom = jbxvt.scr.chars.height - 1;
	} else { // set
		LOG("DECSTBM set");
		LOG("m.b: %d, c.y: %d", s->margin.bottom, s->cursor.y);
		s->margin.top = t[0] - 1;
		s->margin.bottom = t[1] - 1;
	}

}

static void handle_dsr(const int16_t arg)
{
	switch (arg) {
	case 6 :
		cursor(CURSOR_REPORT);
		break;
	case 7 :
		//  Send the name of the display to the command.
		cprintf("%s\r", getenv("DISPLAY"));
		break;
	case 5: // command from host requesting status
		// 0 is response for 'Ready, no malfunctions'
	case 15: // Test printer status
	case 25: // Test UDK status?
	case 26: // Test keyboard status
	default:
		cprintf("\033[0n");
	}
}

static void parse_token(void)
{
	Token token;
	int32_t n; // sanitized first token
	int32_t * t; // shortcut to token.arg
	static bool size_set;
	VTScreen * s;
	s = jbxvt.scr.current; // update in case screen changed
	get_token(&token);
	t = token.arg;
	// n is sanitized for ops with optional args
	n = t[0] ? t[0] : 1;
	switch (token.type) {
	case TK_STRING :
		scr_string(token.string, token.length,
			token.nlcount);
		break;
	case TK_CHAR :
		handle_tk_char(token.tk_char);
		break;
	case TK_EOF :
		LOG("TK_EOF");
		exit(0);
		break;
	case TK_ENTRY :	// keyboard focus changed
		cursor(t[0] ? CURSOR_ENTRY_IN : CURSOR_ENTRY_OUT);
		break;
	case TK_FOCUS :
		if (jbxvt.mode.mouse_focus_evt)
			cprintf("\033[%c]", t[0] ? 'I' : 'O');
		cursor(t[0] ? CURSOR_FOCUS_IN : CURSOR_FOCUS_OUT);
		break;
	case TK_EXPOSE: // window exposed
		expose(token.region, size_set);
		size_set = true;
		break;
	case TK_RESIZE :
		size_set = false;
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
		scroll(s->margin.top,
			s->margin.bot, t[0]);
		break;
	case TK_SBDOWN :
		t[0] = - t[0];
		// fall through
	case TK_SBUP :
		change_offset(jbxvt.scr.offset - t[0]
			/ jbxvt.X.f.size.height);
		break;
	case TK_SELSTART :
		scr_start_selection((xcb_point_t){t[0], t[1]}, SEL_CHAR);
		break;
	case TK_SELEXTND :
		scr_extend_selection((xcb_point_t){t[0], t[1]}, false);
		break;
	case TK_SELDRAG :
		scr_extend_selection((xcb_point_t){t[0], t[1]}, true);
		break;
	case TK_SELWORD :
		scr_start_selection((xcb_point_t){t[0], t[1]}, SEL_WORD);
		break;
	case TK_SELLINE :
		scr_start_selection((xcb_point_t){t[0], t[1]}, SEL_LINE);
		break;
	case TK_SELECT :
		LOG("TK_SELECT");
		scr_make_selection();
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
		request_selection(t[0]);
		break;
	case TK_SELNOTIFY :
		LOG("TK_SELNOTIFY");
		// arg 0 is time, unused
		paste_primary(t[1], t[2]);
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
		scr_move(n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUB: // cursor back
		LOG("TK_CUB");
		scr_move(-n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CPL: // cursor previous line
		LOG("TK_CPL");
		n = -n; // fall through
	case TK_CNL: // cursor next line
		LOG("TK_CNL");
		scr_move(0, n, 0);
		break;
	case TK_HVP: // horizontal vertical position
		LOG("TK_HVP");
		// fall through
	case TK_CUP: // position cursor
		LOG("TK_CUP");
		scr_move(t[1] - 1, t[0] - 1, jbxvt.mode.decom
			? ROW_RELATIVE | COL_RELATIVE : 0);
		break;
	case TK_HPA: // horizontal position absolute
		LOG("TK_HPA");
		scr_move(t[0] - 1, 0, ROW_RELATIVE);
		break;
	case TK_HPR: // horizontal position relative
		LOG("TK_VPA");
		scr_move(t[0] - 1, 0, COL_RELATIVE | ROW_RELATIVE);
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
	case TK_IL:
		n = -n; // fall through
	case TK_DL:
		LOG("TK_IL(-)/TK_DL(+): %d", n);
		scr_index_from(n, s->cursor.y);
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
		dec_reset(&token);
		break;
	case TK_SGR :
		handle_sgr(&token);
		break;
	case TK_DSR: // request for information
		LOG("TK_DSR");
		handle_dsr(t[0]);
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
	case TK_IND: // Index (same as \n)
		LOG("TK_IND");
		scr_index_from(1, s->margin.t);
		break;
	case TK_RI: // Reverse index
		LOG("TK_RI");
		scr_index_from(-1, s->margin.t);
		break;
	case TK_DECID:
	case TK_DA:
		LOG("TK_DECID");
		cprintf("\033[?6c"); // VT102
		break;
	case TK_DECSWH: // ESC # digit
		LOG("TK_DECSWH");
		if (t[0] == '8') // DECALN
			  scr_efill();
		break;
	case TK_NEL : // move to first position on next line down.
		LOG("TK_NEL: NExt Line");
		scr_move(0, s->cursor.y + 1, 0);
		break;
	case TK_DECREQTPARAM:
		LOG("TK_DECREQTPARAM");
		// Send DECREPTPARAM
		switch (t[0]) {
		case 0:
		case 1: {
			const uint8_t sol = t[0] + 2, par = 1, nbits = 1,
			      flags = 0, clkmul = 1;
			const uint16_t xspeed = 88, rspeed = 88;
			cprintf("\033[%d;%d;%d;%d;%d;%d;%dx", sol, par, nbits,
				xspeed, rspeed, clkmul, flags);
			LOG("ESC[%d;%d;%d;%d;%d;%d;%dx", sol, par, nbits,
				xspeed, rspeed, clkmul, flags);
			break;
		}
		default:
			LOG("DECREQTPARAM, unhandled argument %d", t[0]);
		}
		break;
	case TK_DECELR:
		LOG("TK_DECELR");
		jbxvt.opt.elr = t[0] | (t[1] <<2);
		break;
	case TK_RIS: // Reset Initial State
		LOG("TK_RIS");
		scr_reset();
		break;
	case TK_DECSAVEPM: // Save private modes
		LOG("TK_DECSAVEPM");
		memcpy(&jbxvt.saved_mode, &jbxvt.mode,
			sizeof(struct JBXVTPrivateModes));
#if 0
		// FIXME: Unimplemented
		// At least set char set back to ASCII
		// This fixes the links browser
		jbxvt.mode.charset[0] = CHARSET_ASCII;
		jbxvt.mode.charsel = 0;
#endif
		break;
	case TK_SCS0: // DEC SCS G0
		LOG("TK_SCS0");
		select_charset(t[0], 0);
		break;
	case TK_SCS1: // DEC SCS G1
		LOG("TK_SCS1");
		select_charset(t[0], 1);
		break;
	case TK_HTS: // set tab stop at current position
		LOG("TK_HTS");
		scr_set_tab(s->cursor.x, true);
		break;
	case TK_SS2 :
		LOG("TK_SS2");
		break;
	case TK_SS3 :
		LOG("TK_SS3");
		break;
	case TK_TBC: // Tabulation clear
		LOG("TK_TBC");
		switch (t[0]) {
		case 0: // clear at current position
			scr_set_tab(s->cursor.x, false);
			break;
		case 3: // clear all
			scr_set_tab(-1, false);
			break;
		}
		break;
	case TK_DECPM:
		LOG("TK_DECPM");
		s->decpm = true;
		break;
	case TK_DECST:
		LOG("TK_DECST");
		s->decpm = false;
		break;
	case TK_DECLL:
		switch (t[1]) {
		case ' ': // DECSCUSR
			LOG("DECSCUSR");
			jbxvt.opt.cursor_attr = t[0];
			break;
		case '"': // DECSCA
			LOG("DECSCA -- unimplemented");
			break;
		default: // DECLL
			LOG("DECLL -- unimplemented");
		}
		break;
	default:
#ifdef DEBUG
		if(token.type) { // Ignore TK_NULL
			LOG("Unhandled token: %d", token.type);
			exit(1);
		}
#endif//DEBUG
		break;
	}
}

void jbxvt_app_loop(void)
{
	for(;;)
		parse_token();
}

