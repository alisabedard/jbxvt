/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#define LOG_LEVEL 2
#if LOG_LEVEL == 0
#undef DEBUG
#endif//LOG_LEVEL
#include "xvt.h"
#include <stdint.h>
#include <stdio.h>
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
#include "JBXVTToken.h"
#include "JBXVTTokenType.h"
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "dec_reset.h"
#include "double.h"
#include "dsr.h"
#include "edit.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "mode.h"
#include "sbar.h"
#include "erase.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "sgr.h"
#include "string.h"
#include "screen.h"
#include "scroll.h"
#include "selreq.h"
#include "size.h"
#include "tab.h"
#include "tk_char.h"
#include "window.h"
#if LOG_LEVEL > 6
#define TLOG(...) LOG(__VA_ARGS__)
#else//LOG_LEVEL<=6
#define TLOG(...)
#endif//LOG_LEVEL>6
// Return a default of 1 if arg is 0:
__attribute__((const))
static int16_t get_n(const int16_t arg)
{
	return arg ? arg : 1;
}
// Return the 0-based coordinate value:
__attribute__((const))
static int16_t get_0(int16_t arg)
{
	return get_n(arg) - 1;
}
static void handle_token_elr(struct JBXVTToken * restrict token)
{
	int16_t * restrict t = token->arg;
	struct JBXVTPrivateModes * restrict m = jbxvt_get_modes();
	switch (t[0]) {
	case 2:
		m->elr_once = true;
	case 1:
		m->elr = true;
		break;
	default:
		m->elr = m->elr_once = false;
	}
	m->elr_pixels = t[1] == 1;
}
static void handle_token_ll(struct JBXVTToken * restrict token)
{
	int16_t * restrict t = token->arg;
	LOG("t[0]: %d, t[1]: %d", t[0], t[1]);
	switch (t[1]) {
	case 0:
	case ' ': // SCUSR
		LOG("SCUSR");
		jbxvt_set_cursor_attr(t[0]);
		break;
	case '"': // SCA
		LOG("SCA -- unimplemented");
		break;
	default: // LL
		LOG("LL -- unimplemented");
	}
}
static void handle_token_rqm(struct JBXVTToken * restrict token)
{
	int16_t * restrict t = token->arg;
	if (token->private == '?') {
		LOG("\tRQM Ps: %d", t[0]);
		dprintf(jbxvt_get_fd(), "%s%d;%d$y", jbxvt_get_csi(),
			t[0], 0); // FIXME:  Return actual value
		return;
	}
	LOG("\tDECSCL 0: %d, 1: %d", t[0], t[1]);
	switch (t[0]) {
	case 62:
		LOG("\t\tVT200");
		break;
	case 63:
		LOG("\t\tVT300");
		break;
	case 0:
	case 61:
	default:
		LOG("\t\tVT100");
		break;
	}
	if (t[1] == 1) {
		LOG("\t\t7-bit controls");
		jbxvt_get_modes()->s8c1t = false;
	} else {
		LOG("\t\t8-bit controls");
		jbxvt_get_modes()->s8c1t = true;
	}
}
static void handle_token_mc_private(int16_t * restrict t)
{
	switch (t[0]) {
	case 4:
		LOG("turn off printer controller mode");
		break;
	case 5:
		LOG("turn on printer controller mode");
		break;
	case 10:
		LOG("html screen dump");
		break;
	case 11:
		LOG("svg screen dump");
		break;
	case 0:
	default:
		LOG("print screen");
	}
}
static void handle_token_mc_public(int16_t * restrict t)
{
	switch (t[0]) {
	case 1:
		LOG("print line containing cursor");
		break;
	case 4:
		LOG("turn off autoprint mode");
		break;
	case 5:
		LOG("turn on autoprint mode");
		break;
	case 10:
		LOG("print composed display");
		break;
	case 11:
		LOG("print all pages");
		break;
	}
}
static void handle_token_mc(struct JBXVTToken * restrict token)
{
	int16_t * restrict t = token->arg;
	if (token->private != '?')
		handle_token_mc_private(t);
	else
		handle_token_mc_public(t);

}
static void handle_txtpar(xcb_connection_t * xc,
	struct JBXVTToken * restrict token)
{
	switch (token->arg[0]) {
	case 0 :
		jbxvt_change_name(xc, token->string, false);
		jbxvt_change_name(xc, token->string, true);
		break;
	case 1 :
		jbxvt_change_name(xc, token->string, true);
		break;
	case 2 :
		jbxvt_change_name(xc, token->string, false);
		break;
	}
}
static void select_charset(const char c, const uint8_t i)
{
	switch(c) {
#define CS(l, cs, d) case l:TLOG(d);\
		jbxvt_get_modes()->charset[i]=CHARSET_##cs;break;
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
static void decstbm(struct JBXVTToken * restrict token)
{
	int16_t * restrict t = token->arg;
	LOG("JBXVT_TOKEN_STBM args: %d, 0: %d, 1: %d",
		(int)token->nargs, t[0], t[1]);
	if (token->private == JBXVT_TOKEN_RESTOREPM) {
		jbxvt_restore_modes();
		return;
	}
	const bool rst = token->nargs < 2 || t[0] >= t[1];
	struct JBDim * restrict m = jbxvt_get_margin();
	m->top = rst ? 0 : get_0(t[0]);
	m->bot = (rst ? jbxvt_get_char_size().h : get_n(t[1])) - 1;
}
static void reqtparam(const uint8_t t)
{
	// Send REPTPARAM
	const uint8_t sol = t + 2, par = 1, nbits = 1,
	      flags = 0, clkmul = 1;
	const uint16_t xspeed = 88, rspeed = 88;
	dprintf(jbxvt_get_fd(), "%s[%d;%d;%d;%d;%d;%d;%dx", jbxvt_get_csi(),
		sol, par, nbits, xspeed, rspeed, clkmul, flags);
	LOG("ESC[%d;%d;%d;%d;%d;%d;%dx", sol, par, nbits,
		xspeed, rspeed, clkmul, flags);
}
static void tbc(const uint8_t t)
{
	/* Note:  Attempting to simplify this results
	   in vttest test failure.  */
	if (t == 3)
		jbxvt_set_tab(-1, false);
	else if (!t)
		jbxvt_set_tab(jbxvt_get_x(), false);
}
static void handle_scroll(xcb_connection_t * xc, const int16_t arg)
{
	const struct JBDim m = *jbxvt_get_margin();
	// scroll arg lines within margin m:
	scroll(xc, m.top, m.bot, arg);
}
// CUP and HVP, move cursor
static void cup(xcb_connection_t * xc, int16_t * restrict t)
{
	// subtract 1 for 0-based coordinates
	enum {
		COL, ROW, REL = JBXVT_ROW_RELATIVE | JBXVT_COLUMN_RELATIVE,
	};
	const int16_t row = get_0(t[ROW]), col = get_0(t[COL]);
	jbxvt_move(xc, row, col, jbxvt_get_modes()->decom ? REL : 0);
}
// vertical position, absolute or relative
static void vp(xcb_connection_t * xc, const uint16_t arg, const bool relative)
{
	jbxvt_move(xc, 0, get_0(arg), JBXVT_COLUMN_RELATIVE | (relative
		? JBXVT_ROW_RELATIVE : 0));
}
// print terminal id
static void decid(void)
{
	dprintf(jbxvt_get_fd(), "%s?6c", jbxvt_get_csi()); // VT102
}
// set vt52 graphics mode
static void gm52(const bool set)
{
	struct JBXVTPrivateModes * m = jbxvt_get_modes();
	m->charsel = set ? 1 : 0;
	m->gm52 = set;
}
bool jbxvt_parse_token(xcb_connection_t * xc)
{
	struct JBXVTToken token;
	jbxvt_get_token(xc, &token);
	int16_t * t = token.arg;
	// n is sanitized for ops with optional args
	int16_t n = token.nargs ? (get_n(t[0])) : 1;
	switch (token.type) {
	case JBXVT_TOKEN_ALN: // screen alignment test
		LOG("JBXVT_TOKEN_ALN");
		jbxvt_efill(xc);
		break;
	case JBXVT_TOKEN_APC:
		LOG("FIXME JBXVT_TOKEN_APC");
		break;
	case JBXVT_TOKEN_CHA: // cursor CHaracter Absolute column
		LOG("JBXVT_TOKEN_CHA");
		jbxvt_move(xc, get_0(*t), 0, JBXVT_ROW_RELATIVE);
		break;
	case JBXVT_TOKEN_CHAR: // don't log
		jbxvt_handle_tk_char(xc, token.tk_char);
		break;
	case JBXVT_TOKEN_CHT: // change tab stop
		LOG("JBXVT_TOKEN_CHT");
		jbxvt_cht(xc, n);
		break;
	case JBXVT_TOKEN_CPL: // cursor previous line
		LOG("JBXVT_TOKEN_CPL");
		jbxvt_move(xc, 0, -n, 0);
		break;
	case JBXVT_TOKEN_CNL: // cursor next line
		LOG("JBXVT_TOKEN_CNL");
		jbxvt_move(xc, 0, n, 0);
		break;
	case JBXVT_TOKEN_CUB: // left
		LOG("JBXVT_TOKEN_CUB");
		jbxvt_move(xc, -n, 0, JBXVT_ROW_RELATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_CUD: // down
		LOG("JBXVT_TOKEN_CUD");
		jbxvt_move(xc, 0, n, JBXVT_ROW_RELATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_CUF: // right
		LOG("JBXVT_TOKEN_CUF");
		jbxvt_move(xc, n, 0, JBXVT_ROW_RELATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_CUP:
	case JBXVT_TOKEN_HVP:
		TLOG("JBXVT_TOKEN_HVP/JBXVT_TOKEN_CUP");
		cup(xc, t);
		break;
	case JBXVT_TOKEN_CUU: // up
		LOG("JBXVT_TOKEN_CUU");
		jbxvt_move(xc, 0, -n, JBXVT_ROW_RELATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_DA:
	case JBXVT_TOKEN_ID:
		LOG("JBXVT_TOKEN_ID/DA");
		decid();
		break;
	case JBXVT_TOKEN_DCH:
	case JBXVT_TOKEN_ECH:
		LOG("JBXVT_TOKEN_ECH/JBXVT_TOKEN_DCH");
		jbxvt_edit_characters(xc, n, true);
		break;
	case JBXVT_TOKEN_DHLT: // double height line -- top
		LOG("FIXME JBXVT_TOKEN_DHLT");
		break;
	case JBXVT_TOKEN_DHLB: // double height line -- bottom
		LOG("FIXME JBXVT_TOKEN_DHLB");
		break;
	case JBXVT_TOKEN_DL: // delete line
		LOG("JBXVT_TOKEN_DL");
		jbxvt_index_from(xc, n, jbxvt_get_y());
		break;
	case JBXVT_TOKEN_DSR: // request for information
		LOG("JBXVT_TOKEN_DSR");
		jbxvt_handle_dsr(t[0]);
		break;
	case JBXVT_TOKEN_DWL: // double width line
		LOG("JBXVT_TOKEN_DWL");
		jbxvt_set_double_width_line(xc, true);
		break;
	case JBXVT_TOKEN_ED: // erase display
		LOG("JBXVT_TOKEN_ED");
		jbxvt_erase_screen(xc, t[0]); // don't use n
		break;
	case JBXVT_TOKEN_EL: // erase line
		LOG("JBXVT_TOKEN_EL");
		jbxvt_erase_line(xc, t[0]); // don't use n
		break;
	case JBXVT_TOKEN_ELR: // locator report
		LOG("JBXVT_TOKEN_ELR");
		handle_token_elr(&token);
		break;
	case JBXVT_TOKEN_ENTGM52: // vt52 graphics mode
		LOG("JBXVT_TOKEN_ENTGM52");
		gm52(true);
		break;
	case JBXVT_TOKEN_EOF:
		LOG("JBXVT_TOKEN_EOF");
		return false;
	case JBXVT_TOKEN_EPA:
		LOG("FIXME JBXVT_TOKEN_EPA");
		break;
	case JBXVT_TOKEN_EXTGM52: // exit vt52 graphics mode
		LOG("JBXVT_TOKEN_EXTGM52");
		gm52(false);
		break;
	case JBXVT_TOKEN_HOME:
		LOG("JBXVT_TOKEN_HOME");
		jbxvt_set_scroll(xc, 0);
		jbxvt_move(xc, 0, 0, 0);
		break;
	case JBXVT_TOKEN_HPA: // horizontal position absolute
		LOG("JBXVT_TOKEN_HPA");
		jbxvt_move(xc, get_0(*t), 0, JBXVT_ROW_RELATIVE);
		break;
	case JBXVT_TOKEN_HPR: // horizontal position relative
		LOG("JBXVT_TOKEN_HPR");
		jbxvt_move(xc, get_0(*t), 0,
			JBXVT_COLUMN_RELATIVE | JBXVT_ROW_RELATIVE);
		break;
	case JBXVT_TOKEN_HTS: // set tab stop at current position
		LOG("JBXVT_TOKEN_HTS");
		jbxvt_set_tab(jbxvt_get_x(), true);
		break;
	case JBXVT_TOKEN_ICH: // Insert blank characters
		LOG("JBXVT_TOKEN_ICH");
		jbxvt_edit_characters(xc, n, false);
		break;
	case JBXVT_TOKEN_IL: // insert line
		LOG("JBXVT_TOKEN_IL");
		jbxvt_index_from(xc, -n, jbxvt_get_y());
		break;
	case JBXVT_TOKEN_IND: // Index -- same as \n:
		jbxvt_index_from(xc, n, jbxvt_get_margin()->t);
		break;
	case JBXVT_TOKEN_LL:
		LOG("JBXVT_TOKEN_LL");
		handle_token_ll(&token);
		break;
	case JBXVT_TOKEN_MC:
		LOG("JBXVT_TOKEN_MC");
		handle_token_mc(&token);
		break;
	case JBXVT_TOKEN_NEL: // next line
		LOG("JBXVT_TOKEN_NEL");
		// move to first position on next line down.
		jbxvt_move(xc, 0, jbxvt_get_y() + 1, 0);
		break;
	case JBXVT_TOKEN_OSC: // operating system command
		LOG("FIXME JBXVT_TOKEN_OSC");
		break;
	case JBXVT_TOKEN_PAM: // application mode keys
		LOG("JBXVT_TOKEN_PAM");
		jbxvt_set_keys(true, false);
		break;
	case JBXVT_TOKEN_PM: // privacy message
		LOG("JBXVT_TOKEN_PM");
		jbxvt_get_current_screen()->decpm = true;
		break;
	case JBXVT_TOKEN_PNM: // numeric key mode
		LOG("JBXVT_TOKEN_PNM");
		jbxvt_set_keys(false, false);
		break;
	case JBXVT_TOKEN_RC:
		LOG("JBXVT_TOKEN_RC");
		jbxvt_restore_cursor(xc);
		break;
	case JBXVT_TOKEN_REQTPARAM: // request terminal parameters
		LOG("JBXVT_TOKEN_REQTPARAM");
		reqtparam(*t);
		break;
	case JBXVT_TOKEN_RESET:
	case JBXVT_TOKEN_SET:
		TLOG("JBXVT_TOKEN_RESET/JBXVT_TOKEN_SET");
		jbxvt_dec_reset(xc, &token);
		break;
	case JBXVT_TOKEN_RI: // Reverse index
		LOG("JBXVT_TOKEN_RI");
		jbxvt_index_from(xc, -n, jbxvt_get_margin()->top);
		break;
	case JBXVT_TOKEN_RIS: // reset to initial state
		LOG("JBXVT_TOKEN_RIS");
		jbxvt_get_modes()->dectcem = true;
		jbxvt_reset(xc);
		break;
	case JBXVT_TOKEN_RQM:
		LOG("JBXVT_TOKEN_RQM");
		handle_token_rqm(&token);
		break;
	case JBXVT_TOKEN_S7C1T: // 7-bit controls
		LOG("JBXVT_TOKEN_S7C1T");
		jbxvt_get_modes()->s8c1t = false;
		break;
	case JBXVT_TOKEN_S8C1T: // 8-bit controls
		LOG("JBXVT_TOKEN_S8C1T");
		jbxvt_get_modes()->s8c1t = true;
		break;
	case JBXVT_TOKEN_SAVEPM: // Save private modes
		LOG("JBXVT_TOKEN_SAVEPM");
		jbxvt_save_modes();
		break;
	case JBXVT_TOKEN_SBSWITCH:
		LOG("JBXVT_TOKEN_SBSWITCH");
		jbxvt_toggle_scrollbar(xc);
		break;
	case JBXVT_TOKEN_SBGOTO:
		LOG("JBXVT_TOKEN_SBGOTO");
		/*  Move the display so that line represented
		    by scrollbar value is at the top of the screen.  */
		jbxvt_scroll_to(xc, *t);
		break;
	case JBXVT_TOKEN_SC:
		LOG("JBXVT_TOKEN_SC");
		jbxvt_save_cursor();
		break;
	case JBXVT_TOKEN_CS_G0:
#if LOG_LEVEL > 6
		LOG("JBXVT_TOKEN_CS_G0");
#endif//LOG_LEVEL>6
		select_charset(*t, 0);
		break;
	case JBXVT_TOKEN_CS_G1:
	case JBXVT_TOKEN_CS_ALT_G1:
#if LOG_LEVEL > 6
		LOG("JBXVT_TOKEN_CS_G1");
#endif//LOG_LEVEL>6
		select_charset(*t, 1);
		break;
	case JBXVT_TOKEN_CS_G2:
	case JBXVT_TOKEN_CS_ALT_G2:
#if LOG_LEVEL > 6
		LOG("JBXVT_TOKEN_CS_G2");
#endif//LOG_LEVEL>6
		select_charset(*t, 2);
		break;
	case JBXVT_TOKEN_CS_G3:
	case JBXVT_TOKEN_CS_ALT_G3:
#if LOG_LEVEL > 6
		LOG("JBXVT_TOKEN_CS_G3");
#endif//LOG_LEVEL>6
		select_charset(*t, 3);
		break;
	case JBXVT_TOKEN_SD:
		LOG("JBXVT_TOKEN_SD");
		// scroll down n lines
		*t = - *t; // fall through
	case JBXVT_TOKEN_SU:
		LOG("JBXVT_TOKEN_SU");
		handle_scroll(xc, *t);
		break;
	case JBXVT_TOKEN_SELINSRT:
		LOG("JBXVT_TOKEN_SELINSRT");
		jbxvt_request_selection(xc, *t);
		break;
	case JBXVT_TOKEN_SPA: // start protected area
		LOG("FIXME JBXVT_TOKEN_SPA");
		break;
	case JBXVT_TOKEN_SS2:
		LOG("JBXVT_TOKEN_SS2");
		jbxvt_get_modes()->ss2 = true;
		break;
	case JBXVT_TOKEN_SS3:
		LOG("JBXVT_TOKEN_SS3");
		jbxvt_get_modes()->ss3 = true;
		break;
	case JBXVT_TOKEN_STRING: // don't log
#ifdef JBXVT_DEBUG_STRING
		LOG("token.length: %d", token.length);
#endif//JBXVT_DEBUG_STRING
		jbxvt_string(xc, token.string, token.length,
			token.nlcount);
		break;
	case JBXVT_TOKEN_TXTPAR:
		LOG("JBXVT_TOKEN_TXTPAR");
		// change title or icon name
		handle_txtpar(xc, &token);
		break;
	case JBXVT_TOKEN_SGR:
		TLOG("JBXVT_TOKEN_SGR");
		jbxvt_handle_sgr(xc, &token);
		break;
	case JBXVT_TOKEN_SOS: // start of string
		LOG("FIXME JBXVT_TOKEN_SOS");
		break;
	case JBXVT_TOKEN_ST: // string terminator
		LOG("JBXVT_TOKEN_ST");
		jbxvt_get_current_screen()->decpm = false;
		break;
	case JBXVT_TOKEN_STBM: // set top and bottom margins.
		LOG("JBXVT_TOKEN_STBM");
		decstbm(&token);
		break;
	case JBXVT_TOKEN_SWL: // single width line
		LOG("JBXVT_TOKEN_SWL");
		jbxvt_set_double_width_line(xc, false);
		break;
	case JBXVT_TOKEN_TBC: // Tabulation clear
		LOG("JBXVT_TOKEN_TBC");
		tbc(t[0]);
		break;
	case JBXVT_TOKEN_VPA: // vertical position absolute
		LOG("JBXVT_TOKEN_VPA");
		vp(xc, *t, false);
		break;
	case JBXVT_TOKEN_VPR: // vertical position relative
		LOG("JBXVT_TOKEN_VPR");
		vp(xc, *t, true);
		break;
	default:
#ifdef DEBUG
		if(token.type) { // Ignore JBXVT_TOKEN_NULL
			LOG("Unhandled token: %d (0x%x)",
				token.type, token.type);
			// Exit now so we can implement it!
			return false;
		}
#endif//DEBUG
		break;
	}
	return true;
}
