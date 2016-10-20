/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "xvt.h"
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "dec_reset.h"
#include "double.h"
#include "handle_sgr.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "repaint.h"
#include "sbar.h"
#include "edit.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_string.h"
#include "screen.h"
#include "scroll.h"
#include "selreq.h"
#include "window.h"
#include <string.h>
//#define DEBUG_TOKENS
#ifdef DEBUG_TOKENS
#define TLOG(...) LOG(__VA_ARGS__)
#else
#define TLOG(...)
#endif//DEBUG_TOKENS
static void handle_txtpar(struct Token * restrict token)
{
	switch (token->arg[0]) {
	case 0 :
#define CN(b) jbxvt_change_name(token->string, b)
		CN(false);
		CN(true);
		break;
	case 1 :
		CN(true);
		break;
	case 2 :
		CN(false);
		break;
	}
}
static void form_feed(void)
{
	const struct JBDim m = jbxvt.scr.current->margin;
	jbxvt_move(0, m.top, 0);
	if (jbxvt.mode.decpff)
		cprintf("FF");
	scroll(m.top, m.bottom, m.bottom - m.top);
}
static void handle_tk_char(const uint8_t tk_char)
{
	switch (tk_char) {
	case '\n': // handle line feed
		jbxvt_index_from(1, jbxvt.scr.current->margin.t);
		break;
	case 013: // vertical tab
		for (uint8_t i = jbxvt.scr.current->cursor.y; i % 8; ++i)
			  jbxvt_index_from(1, jbxvt.scr.current->margin.t);
		break;
	case '\f': // form feed
		form_feed();
		break;
	case '\r': // handle carriage return
		jbxvt_move(0,0,ROW_RELATIVE);
		break;
	case '\b': // handle a backspace
		jbxvt_move(-1,0,COL_RELATIVE|ROW_RELATIVE);
		break;
	case '\t': // handle tab
		jbxvt_tab();
		break;
	case 005: // ENQ
		cprintf("\033[?6c"); // VT102
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
static void select_charset(const char c, const uint8_t i)
{
	switch(c) {
#define CS(l, cs, d) case l:LOG(d);jbxvt.mode.charset[i]=CHARSET_##cs;break;
	CS('A', GB, "UK ASCII");
	CS('0', SG0, "SG0: special graphics");
	CS('1', SG1, "SG1: alt char ROM standard graphics");
	CS('2', SG2, "SG2: alt char ROM special graphics");
	default: // reset
		LOG("Unknown character set");
		// fall through
	CS('B', ASCII, "US ASCII");
	}
}
static void decstbm(struct Token * restrict token)
{
	int32_t * restrict t = token->arg;
	LOG("JBXVT_TOKEN_STBM args: %d, 0: %d, 1: %d",
		(int)token->nargs, t[0], t[1]);
	if (token->private == JBXVT_TOKEN_RESTOREPM) {
		// Restore private modes.
		memcpy(&jbxvt.mode, &jbxvt.saved_mode,
			sizeof(struct JBXVTPrivateModes));
		return;
	}
	const bool rst = token->nargs < 2 || t[0] >= t[1];
	jbxvt.scr.current->margin = (struct JBDim){.t = rst ? 0 : t[0] - 1,
		.b = (rst ? jbxvt.scr.chars.h : t[1]) - 1};
}
static void handle_dsr(const int16_t arg)
{
	switch (arg) {
	case 6 : {
		const struct JBDim c = jbxvt.scr.current->cursor;
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
		jbxvt_set_tab(-1, false);
	else if (!t)
		jbxvt_set_tab(jbxvt.scr.current->cursor.x, false);
}
void jbxvt_parse_token(void)
{
	struct Token token;
	get_token(&token);
	int32_t * t = token.arg;
	// n is sanitized for ops with optional args
	int32_t n = token.nargs ? (t[0] ? t[0] : 1) : 1;
	switch (token.type) {
// macro to aid in debug logging
#define CASE(L) case L:TLOG(#L);
// log unimplemented features
#define FIXME(L) CASE(L);TLOG("\tFIXME: " #L " Unimplemented");break;
	CASE(JBXVT_TOKEN_ALN) // screen alignment test
		jbxvt_efill();
		break;
	FIXME(JBXVT_TOKEN_APC);
	CASE(JBXVT_TOKEN_CHA) // cursor CHaracter Absolute column
		jbxvt_move(t[0] - 1, 0, ROW_RELATIVE);
		break;
	case JBXVT_TOKEN_CHAR: // don't log
		handle_tk_char(token.tk_char);
		break;
	CASE(JBXVT_TOKEN_CHT);
		jbxvt_cht(n);
		break;
	CASE(JBXVT_TOKEN_CPL) // cursor previous line
		n = -n; // fall through
	CASE(JBXVT_TOKEN_CNL) // cursor next line
		LOG("JBXVT_TOKEN_CNL");
		jbxvt_move(0, n, 0);
		break;
	CASE(JBXVT_TOKEN_CUB); // back
		jbxvt_move(-n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	CASE(JBXVT_TOKEN_CUD); // down
		jbxvt_move(0, n, ROW_RELATIVE | COL_RELATIVE);
		break;
	CASE(JBXVT_TOKEN_CUF); // forward
		jbxvt_move(n, 0, ROW_RELATIVE | COL_RELATIVE);
		break;
	CASE(JBXVT_TOKEN_CUP) // fall through
	CASE(JBXVT_TOKEN_HVP)
		// subtract 1 for 0-based coordinates
		jbxvt_move((t[1]?t[1]:1) - 1, n - 1, jbxvt.mode.decom ?
			ROW_RELATIVE | COL_RELATIVE : 0);
		break;
	CASE(JBXVT_TOKEN_CUU); // up
		jbxvt_move(0, -n, ROW_RELATIVE | COL_RELATIVE);
		break;
	CASE(JBXVT_TOKEN_DA) // fall through
	CASE(JBXVT_TOKEN_ID)
		LOG("JBXVT_TOKEN_ID");
		cprintf("\033[?6c"); // VT102
		break;
	CASE(JBXVT_TOKEN_DCH) // fall through
	CASE(JBXVT_TOKEN_ECH)
		jbxvt_delete_characters(n);
		break;
	FIXME(JBXVT_TOKEN_DHLT); // double height line -- top
	FIXME(JBXVT_TOKEN_DHLB); // double height line -- bottom
	CASE(JBXVT_TOKEN_DSR) // request for information
		handle_dsr(t[0]);
		break;
	CASE(JBXVT_TOKEN_DWL) // double width line
		jbxvt_set_double_width_line(true);
		break;
	CASE(JBXVT_TOKEN_ED) // erase display
		jbxvt_erase_screen(t[0]); // don't use n
		break;
	CASE(JBXVT_TOKEN_EL) // erase line
		jbxvt_erase_line(t[0]); // don't use n
		break;
	CASE(JBXVT_TOKEN_ENTGM52)
		jbxvt.mode.charsel = 1;
		jbxvt.mode.gm52 = true;
		break;
	CASE(JBXVT_TOKEN_ELR)
		switch (t[0]) {
		case 2:
			jbxvt.mode.elr_once = true;
		case 1:
			jbxvt.mode.elr = true;
			break;
		case 0:
		default:
			jbxvt.mode.elr = false;
			jbxvt.mode.elr_once = false;
		}
		jbxvt.mode.elr_pixels = t[1] == 1;
		break;
	CASE(JBXVT_TOKEN_ENTRY) // keyboard focus changed.  fall through:
	CASE(JBXVT_TOKEN_FOCUS) // mouse focus changed
		if (jbxvt.mode.mouse_focus_evt)
			cprintf("\033[%c]", t[0] ? 'I' : 'O');
		break;
	CASE(JBXVT_TOKEN_EOF)
		exit(0);
	FIXME(JBXVT_TOKEN_EPA);
	CASE(JBXVT_TOKEN_EXTGM52)
		jbxvt.mode.charsel = 0;
		jbxvt.mode.gm52 = false;
		break;
	CASE(JBXVT_TOKEN_HOME)
		jbxvt_set_scroll(0);
		jbxvt_move(0, 0, 0);
		break;
	CASE(JBXVT_TOKEN_HPA) // horizontal position absolute
		jbxvt_move(t[0] - 1, 0, ROW_RELATIVE);
		break;
	CASE(JBXVT_TOKEN_HPR) // horizontal position relative
		jbxvt_move(t[0] - 1, 0, COL_RELATIVE | ROW_RELATIVE);
		break;
	CASE(JBXVT_TOKEN_HTS) // set tab stop at current position
		jbxvt_set_tab(jbxvt.scr.current->cursor.x, true);
		break;
	CASE(JBXVT_TOKEN_ICH)
		jbxvt_insert_characters(n);
		break;
	CASE(JBXVT_TOKEN_IL) n = -n; // fall through
	CASE(JBXVT_TOKEN_DL)
		jbxvt_index_from(n, jbxvt.scr.current->cursor.y);
		break;
	CASE(JBXVT_TOKEN_LL)
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
	CASE(JBXVT_TOKEN_NEL) // move to first position on next line down.
		jbxvt_move(0, jbxvt.scr.current->cursor.y + 1, 0);
		break;
	FIXME(JBXVT_TOKEN_OSC);
	CASE(JBXVT_TOKEN_PAM)
		jbxvt_set_keys(true, false);
		break;
	CASE(JBXVT_TOKEN_PM)
		jbxvt.scr.current->decpm = true;
		break;
	CASE(JBXVT_TOKEN_PNM)
		jbxvt_set_keys(false, false);
		break;
	CASE(JBXVT_TOKEN_RC)
		jbxvt_restore_cursor();
		break;
	CASE(JBXVT_TOKEN_REQTPARAM)
		reqtparam(t[0]);
		break;
	CASE(JBXVT_TOKEN_RI) // Reverse index
		n = -n; // fall through
	CASE(JBXVT_TOKEN_IND) // Index (same as \n)
		jbxvt_index_from(n, jbxvt.scr.current->margin.t);
		break;
	CASE(JBXVT_TOKEN_S7C1T) // 7-bit controls
		jbxvt.mode.s8c1t = false;
		break;
	CASE(JBXVT_TOKEN_S8C1T) // 8-bit controls
		jbxvt.mode.s8c1t = true;
		break;
	CASE(JBXVT_TOKEN_SAVEPM) // Save private modes
		memcpy(&jbxvt.saved_mode, &jbxvt.mode,
			sizeof(struct JBXVTPrivateModes));
		break;
	CASE(JBXVT_TOKEN_SBSWITCH)
		jbxvt_toggle_scrollbar();
		break;
	CASE(JBXVT_TOKEN_SBGOTO)
		/*  Move the display so that line represented by scrollbar value
		    is at the top of the screen.  */
		jbxvt_scroll_to(t[0]);
		break;
	CASE(JBXVT_TOKEN_SC)
		jbxvt_save_cursor();
		break;
	CASE(JBXVT_TOKEN_SCS0) //  SCS G0
		select_charset(t[0], 0);
		break;
	CASE(JBXVT_TOKEN_SCS1) //  SCS G1
		select_charset(t[0], 1);
		break;
	CASE(JBXVT_TOKEN_SD) // scroll down n lines
		t[0] = - t[0]; // fall through
	CASE(JBXVT_TOKEN_SU) // scroll up n lines;
		LOG("JBXVT_TOKEN_SU");
		scroll(jbxvt.scr.current->margin.top,
			jbxvt.scr.current->margin.bot, t[0]);
		break;
	CASE(JBXVT_TOKEN_SELINSRT)
		jbxvt_request_selection(t[0]);
		break;
	FIXME(JBXVT_TOKEN_SPA);
	FIXME(JBXVT_TOKEN_SS2);
	FIXME(JBXVT_TOKEN_SS3);
	case JBXVT_TOKEN_STRING: // don't log
		jbxvt_string(token.string, token.length,
			token.nlcount);
		break;
	CASE(JBXVT_TOKEN_TXTPAR)	// change title or icon name
		handle_txtpar(&token);
		break;
	CASE(JBXVT_TOKEN_SET) // fall through
	CASE(JBXVT_TOKEN_RESET)
		jbxvt_dec_reset(&token);
		break;
	CASE(JBXVT_TOKEN_SGR)
		jbxvt_handle_sgr(&token);
		break;
	FIXME(JBXVT_TOKEN_SOS); // start of string
	CASE(JBXVT_TOKEN_ST)
		jbxvt.scr.current->decpm = false;
		break;
	CASE(JBXVT_TOKEN_STBM) // set top and bottom margins.
		decstbm(&token);
		break;
	CASE(JBXVT_TOKEN_SWL) // single width line
		jbxvt_set_double_width_line(false);
		break;
	CASE(JBXVT_TOKEN_TBC) // Tabulation clear
		tbc(t[0]);
		break;
	CASE(JBXVT_TOKEN_VPA) // vertical position absolute
		jbxvt_move(0, t[0] - 1, COL_RELATIVE);
		break;
	CASE(JBXVT_TOKEN_VPR) // vertical position relative
		jbxvt_move(0, t[0] - 1, COL_RELATIVE|ROW_RELATIVE);
		break;
	default:
#ifdef DEBUG
		if(token.type) { // Ignore JBXVT_TOKEN_NULL
			LOG("Unhandled token: %d (0x%x)",
				token.type, token.type);
			// Exit now so we can implement it!
			exit(1);
		}
#endif//DEBUG
		break;
	}
}
