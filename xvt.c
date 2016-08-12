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
		draw_cursor(); // clear
		repaint();
		draw_cursor(); // draw
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
	LOG("TK_STBM args: %d, 0: %d, 1: %d",
		(int)token->nargs, t[0], t[1]);
	VTScreen * restrict s = jbxvt.scr.current;
	if (token->private == '?') { //RESTOREPM
		// Restore private modes
		memcpy(&jbxvt.mode, &jbxvt.saved_mode,
			sizeof(struct JBXVTPrivateModes));
	} else if (token->nargs < 2 || t[0] >= t[1]) {
		LOG("STBM reset");
		// reset
		s->margin.top = 0;
		s->margin.bottom = jbxvt.scr.chars.height - 1;
	} else { // set
		LOG("STBM set");
		LOG("m.b: %d, c.y: %d", s->margin.bottom, s->cursor.y);
		s->margin.top = t[0] - 1;
		s->margin.bottom = t[1] - 1;
	}

}

static void handle_dsr(const int16_t arg)
{
	switch (arg) {
	case 6 : {
		const xcb_point_t c = jbxvt.scr.current->cursor;
		cprintf("\033[%d;%dR", c.y + 1, c.x + 1);
		break;
	}
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

static void reqtparam(const uint8_t t)
{
	// Send REPTPARAM
	const uint8_t sol = t + 2, par = 1, nbits = 1,
	      flags = 0, clkmul = 1;
	const uint16_t xspeed = 88, rspeed = 88;
	cprintf("\033[%d;%d;%d;%d;%d;%d;%dx", sol, par, nbits,
		xspeed, rspeed, clkmul, flags);
	LOG("ESC[%d;%d;%d;%d;%d;%d;%dx", sol, par, nbits,
		xspeed, rspeed, clkmul, flags);
}

static void tbc(const uint8_t t)
{
	if (t == 3)
		scr_set_tab(-1, false);
	else if (!t)
		scr_set_tab(jbxvt.scr.current->cursor.x, false);
}

static void parse_token(void)
{
	Token token;
	int32_t n; // sanitized first token
	int32_t * t; // shortcut to token.arg
	static bool size_set = true;
	VTScreen * s;
	s = jbxvt.scr.current; // update in case screen changed
	get_token(&token);
	t = token.arg;
	// n is sanitized for ops with optional args
	n = t[0] ? t[0] : 1;
	switch (token.type) {
#define CASE(L) break; case L: if (L) { LOG(#L) }
#define FIXME(L) CASE(L); LOG("\tFIXME: Unimplemented");
	case TK_STRING :
		scr_string(token.string, token.length,
			token.nlcount);
		break;
	case TK_CHAR :
		handle_tk_char(token.tk_char);
	CASE(TK_CHT);
		scr_cht(n);
	CASE(TK_CUU); // up
		scr_move(0, -n, ROW_RELATIVE | COL_RELATIVE);
	CASE(TK_CUD); // down
		scr_move(0, n, ROW_RELATIVE | COL_RELATIVE);
	CASE(TK_CUF); // forward
		scr_move(n, 0, ROW_RELATIVE | COL_RELATIVE);
	CASE(TK_CUB); // back
		scr_move(-n, 0, ROW_RELATIVE | COL_RELATIVE);

	CASE(TK_CPL); // cursor previous line
		n = -n; // fall through
	case TK_CNL: // cursor next line
		LOG("TK_CNL");
		scr_move(0, n, 0);
		break;

	case TK_HVP: // horizontal vertical position
		LOG("TK_HVP");
		// fall through
	case TK_CUP: // position cursor
		// subtract 1 for 0-based coordinates
		scr_move((t[1]?t[1]:1) - 1, n - 1, jbxvt.mode.decom ?
			ROW_RELATIVE | COL_RELATIVE : 0);

	CASE(TK_HPA) // horizontal position absolute
		scr_move(t[0] - 1, 0, ROW_RELATIVE);
	CASE(TK_HPR) // horizontal position relative
		scr_move(t[0] - 1, 0, COL_RELATIVE | ROW_RELATIVE);
	CASE(TK_VPA) // vertical position absolute
		scr_move(0, t[0] - 1, COL_RELATIVE);
		break;
	CASE(TK_VPR) // vertical position relative
		scr_move(0, t[0] - 1, COL_RELATIVE|ROW_RELATIVE);
	CASE(TK_CHA) // cursor CHaracter Absolute column
		scr_move(t[0] - 1, 0, ROW_RELATIVE);
	CASE(TK_ED) // erase display
		scr_erase_screen(t[0]); // don't use n
	CASE(TK_EL) // erase line
		scr_erase_line(t[0]); // don't use n
	CASE(TK_EOF)
		LOG("TK_EOF");
		exit(0);
	CASE(TK_ENTGM52)
		jbxvt.mode.charsel = 1;
		jbxvt.mode.gm52 = true;
	CASE(TK_EXTGM52)
		jbxvt.mode.charsel = 0;
		jbxvt.mode.gm52 = false;

	CASE(TK_ENTRY) // keyboard focus changed
		// fall through
	case TK_FOCUS: // mouse focus changed
		if (jbxvt.mode.mouse_focus_evt)
			cprintf("\033[%c]", t[0] ? 'I' : 'O');

	CASE(TK_EXPOSE)
		expose(token.region, size_set);
		size_set = true;
	CASE(TK_HOME)
		change_offset(0);
		scr_move(0, 0, 0);
	CASE(TK_RESIZE)
		size_set = false;
		resize_window();
	CASE(TK_S7C1T) // 7-bit controls
		jbxvt.mode.s8c1t = false;
	CASE(TK_S8C1T) // 8-bit controls
		jbxvt.mode.s8c1t = true;
	CASE(TK_SBSWITCH)
		switch_scrollbar();
	CASE(TK_SBGOTO)
		/*  Move the display so that line represented by scrollbar value
		    is at the top of the screen.  */
		change_offset((jbxvt.scr.chars.height + jbxvt.scr.sline.top)
			* (jbxvt.scr.pixels.height - t[0])
			/ jbxvt.scr.pixels.height - jbxvt.scr.chars.height);

	CASE(TK_SD) // scroll down n lines
		t[0] = - t[0];
		// fall through
	case TK_SU: // scroll up n lines;
		LOG("TK_SU");
		scroll(s->margin.top,
			s->margin.bot, t[0]);
		break;

	CASE(TK_SBDOWN)
		t[0] = - t[0];
		// fall through
	case TK_SBUP :
		change_offset(jbxvt.scr.offset - t[0]
			/ jbxvt.X.f.size.height);

	CASE(TK_SELSTART)
		scr_start_selection((xcb_point_t){t[0], t[1]}, SEL_CHAR);
	CASE(TK_SELEXTND)
		scr_extend_selection((xcb_point_t){t[0], t[1]}, false);
	CASE(TK_SELDRAG)
		scr_extend_selection((xcb_point_t){t[0], t[1]}, true);
	CASE(TK_SELWORD)
		scr_start_selection((xcb_point_t){t[0], t[1]}, SEL_WORD);
	CASE(TK_SELLINE)
		scr_start_selection((xcb_point_t){t[0], t[1]}, SEL_LINE);
	CASE(TK_SELECT)
		scr_make_selection();
	CASE(TK_SELCLEAR)
		scr_clear_selection();
	CASE(TK_SELREQUEST)
		scr_send_selection(t[0], t[1], t[2], t[3]);
	CASE(TK_SELINSRT)
		request_selection(t[0]);
	CASE(TK_SELNOTIFY)
		LOG("TK_SELNOTIFY");
		// arg 0 is time, unused
		paste_primary(t[1], t[2]);
	CASE(TK_TXTPAR)	// change title or icon name
		handle_txtpar(&token);

	CASE(TK_IL)
		n = -n; // fall through
	case TK_DL:
#ifdef DEBUG
		if (n > 0)
			LOG("TK_DL")
#endif//DEBUG
		scr_index_from(n, s->cursor.y);

	CASE(TK_DCH)
		// fall through
	case TK_ECH:
		scr_delete_characters(n);

	CASE(TK_ICH)
		scr_insert_characters(n);

	CASE(TK_SET)
		// fall through
	case TK_RESET :
		LOG("TK_RESET");
		dec_reset(&token);

	CASE(TK_SGR)
		handle_sgr(&token);
	CASE(TK_DSR) // request for information
		handle_dsr(t[0]);
	CASE(TK_STBM) // set top and bottom margins.
		decstbm(&token);
	CASE(TK_SC)
		save_cursor();
	CASE(TK_RC)
		restore_cursor();
	CASE(TK_PAM)
		set_keys(true, false);
	CASE(TK_PNM)
		set_keys(false, false);

	CASE(TK_RI) // Reverse index
		n = -n;
		// fall through
	case TK_IND: // Index (same as \n)
		scr_index_from(n, s->margin.t);

	CASE(TK_ID)
		// fall through
	case TK_DA:
		LOG("TK_ID");
		cprintf("\033[?6c"); // VT102

	CASE(TK_ALN) // screen alignment test
		scr_efill();
	FIXME(TK_DHLT) // double height line -- top
	FIXME(TK_DHLB) // double height line -- bottom
	CASE(TK_SWL) // single width line
		jbxvt.mode.decdwl = true;
	CASE(TK_DWL) // double width line
		jbxvt.mode.decdwl = true;
	CASE(TK_NEL) // move to first position on next line down.
		scr_move(0, s->cursor.y + 1, 0);
	CASE(TK_REQTPARAM)
		reqtparam(t[0]);
	CASE(TK_ELR)
		jbxvt.opt.elr = t[0] | (t[1] <<2);
	FIXME(TK_EPA)
	CASE(TK_HTS) // set tab stop at current position
		scr_set_tab(s->cursor.x, true);
		break;
	FIXME(TK_OSC);
	CASE(TK_RIS) // Reset Initial State
		scr_reset();
	CASE(TK_SCS0) //  SCS G0
		select_charset(t[0], 0);
	CASE(TK_SCS1) //  SCS G1
		select_charset(t[0], 1);
	FIXME(TK_SPA)
	FIXME(TK_SS2)
	FIXME(TK_SS3)
	CASE(TK_PM)
		s->decpm = true;
		break;
	CASE(TK_SAVEPM) // Save private modes
		memcpy(&jbxvt.saved_mode, &jbxvt.mode,
			sizeof(struct JBXVTPrivateModes));
	CASE(TK_ST)
		s->decpm = false;
	CASE(TK_TBC) // Tabulation clear
		tbc(t[0]);
	CASE(TK_LL)
		switch (t[1]) {
		case ' ': // SCUSR
			LOG("SCUSR");
			jbxvt.opt.cursor_attr = t[0];
			break;
		case '"': // SCA
			LOG("SCA -- unimplemented");
			break;
		default: // LL
			LOG("LL -- unimplemented");
		}
		break;
	default:
#ifdef DEBUG
		if(token.type) { // Ignore TK_NULL
			LOG("Unhandled token: %d (0x%x)",
				token.type, token.type);
			// Exit now so we can implement it!
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

