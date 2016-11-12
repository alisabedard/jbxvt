/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "xvt.h"
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "dec_reset.h"
#include "double.h"
#include "dsr.h"
#include "edit.h"
#include "handle_sgr.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "mode.h"
#include "sbar.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "screen.h"
#include "scroll.h"
#include "selreq.h"
#include "size.h"
#include "tab.h"
#include "tk_char.h"
#include "window.h"
//#define DEBUG_TOKENS
#ifdef DEBUG_TOKENS
#define TLOG(...) LOG(__VA_ARGS__)
#else
#define TLOG(...)
#endif//DEBUG_TOKENS
static void handle_token_ll(struct Token * restrict token)
{
	int32_t * restrict t = token->arg;
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
static void handle_token_rqm(struct Token * restrict token)
{
	int32_t * restrict t = token->arg;
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
static void handle_token_mc(struct Token * restrict token)
{
	int32_t * restrict t = token->arg;
	if (token->private != '?')
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
	else
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
static void handle_txtpar(xcb_connection_t * xc,
	struct Token * restrict token)
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
#define CS(l, cs, d) case l:LOG(d);\
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
static void decstbm(struct Token * restrict token)
{
	int32_t * restrict t = token->arg;
	LOG("JBXVT_TOKEN_STBM args: %d, 0: %d, 1: %d",
		(int)token->nargs, t[0], t[1]);
	if (token->private == JBXVT_TOKEN_RESTOREPM) {
		jbxvt_restore_modes();
		return;
	}
	const bool rst = token->nargs < 2 || t[0] >= t[1];
	jbxvt_get_screen()->margin = (struct JBDim){.t = rst ? 0 : t[0] - 1,
		.b = (rst ? jbxvt_get_char_size().h : t[1]) - 1};
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
	if (t == 3)
		jbxvt_set_tab(-1, false);
	else if (!t)
		jbxvt_set_tab(jbxvt_get_screen()->cursor.x, false);
}
void jbxvt_parse_token(xcb_connection_t * xc)
{
	struct Token token;
	jbxvt_get_token(xc, &token);
	int32_t * t = token.arg;
	// n is sanitized for ops with optional args
	int32_t n = token.nargs ? (t[0] ? t[0] : 1) : 1;
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
		jbxvt_move(xc, t[0] - 1, 0, JBXVT_ROW_RELAATIVE);
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
		jbxvt_move(xc, -n, 0, JBXVT_ROW_RELAATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_CUD: // down
		LOG("JBXVT_TOKEN_CUD");
		jbxvt_move(xc, 0, n, JBXVT_ROW_RELAATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_CUF: // right
		LOG("JBXVT_TOKEN_CUF");
		jbxvt_move(xc, n, 0, JBXVT_ROW_RELAATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_CUP:
	case JBXVT_TOKEN_HVP:
		TLOG("JBXVT_TOKEN_HVP/JBXVT_TOKEN_CUP");
		// subtract 1 for 0-based coordinates
		jbxvt_move(xc, (t[1]?t[1]:1) - 1, n - 1,
			jbxvt_get_modes()->decom ?  JBXVT_ROW_RELAATIVE
			| JBXVT_COLUMN_RELATIVE : 0);
		break;
	case JBXVT_TOKEN_CUU: // up
		LOG("JBXVT_TOKEN_CUU");
		jbxvt_move(xc, 0, -n, JBXVT_ROW_RELAATIVE
			| JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_DA:
	case JBXVT_TOKEN_ID:
		LOG("JBXVT_TOKEN_ID/DA");
		dprintf(jbxvt_get_fd(), "%s?6c", jbxvt_get_csi()); // VT102
		break;
	case JBXVT_TOKEN_DCH:
	case JBXVT_TOKEN_ECH:
		LOG("JBXVT_TOKEN_ECH/JBXVT_TOKEN_DCH");
		jbxvt_delete_characters(xc, n);
		break;
	case JBXVT_TOKEN_DHLT: // double height line -- top
		LOG("FIXME JBXVT_TOKEN_DHLT");
		break;
	case JBXVT_TOKEN_DHLB: // double height line -- bottom
		LOG("FIXME JBXVT_TOKEN_DHLB");
		break;
	case JBXVT_TOKEN_DL: // delete line
		LOG("JBXVT_TOKEN_DL");
		jbxvt_index_from(xc, n, jbxvt_get_screen()->cursor.y);
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
		switch (t[0]) {
		case 2:
			jbxvt_get_modes()->elr_once = true;
		case 1:
			jbxvt_get_modes()->elr = true;
			break;
		case 0:
		default:
			jbxvt_get_modes()->elr = false;
			jbxvt_get_modes()->elr_once = false;
		}
		jbxvt_get_modes()->elr_pixels = t[1] == 1;
		break;
	case JBXVT_TOKEN_ENTGM52: // vt52 graphics mode
		LOG("JBXVT_TOKEN_ENTGM52");
		jbxvt_get_modes()->gm52 = true;
		break;
	case JBXVT_TOKEN_EOF:
		LOG("JBXVT_TOKEN_EOF");
		exit(0);
	case JBXVT_TOKEN_EPA:
		LOG("FIXME JBXVT_TOKEN_EPA");
		break;
	case JBXVT_TOKEN_EXTGM52: // exit vt52 graphics mode
		LOG("JBXVT_TOKEN_EXTGM52");
		jbxvt_get_modes()->charsel = 0;
		jbxvt_get_modes()->gm52 = false;
		break;
	case JBXVT_TOKEN_HOME:
		LOG("JBXVT_TOKEN_HOME");
		jbxvt_set_scroll(xc, 0);
		jbxvt_move(xc, 0, 0, 0);
		break;
	case JBXVT_TOKEN_HPA: // horizontal position absolute
		LOG("JBXVT_TOKEN_HPA");
		jbxvt_move(xc, t[0] - 1, 0, JBXVT_ROW_RELAATIVE);
		break;
	case JBXVT_TOKEN_HPR: // horizontal position relative
		LOG("JBXVT_TOKEN_HPR");
		jbxvt_move(xc, t[0] - 1, 0,
			JBXVT_COLUMN_RELATIVE | JBXVT_ROW_RELAATIVE);
		break;
	case JBXVT_TOKEN_HTS: // set tab stop at current position
		LOG("JBXVT_TOKEN_HTS");
		jbxvt_set_tab(jbxvt_get_screen()->cursor.x, true);
		break;
	case JBXVT_TOKEN_ICH: // Insert blank characters
		LOG("JBXVT_TOKEN_ICH");
		jbxvt_insert_characters(xc, n);
		break;
	case JBXVT_TOKEN_IL: // insert line
		LOG("JBXVT_TOKEN_IL");
		jbxvt_index_from(xc, -n, jbxvt_get_screen()->cursor.y);
		break;
	case JBXVT_TOKEN_IND: // Index -- same as \n:
		jbxvt_index_from(xc, n, jbxvt_get_screen()->margin.t);
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
		jbxvt_move(xc, 0, jbxvt_get_screen()->cursor.y + 1, 0);
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
		jbxvt_get_screen()->decpm = true;
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
		reqtparam(t[0]);
		break;
	case JBXVT_TOKEN_RESET:
	case JBXVT_TOKEN_SET:
		TLOG("JBXVT_TOKEN_RESET/JBXVT_TOKEN_SET");
		jbxvt_dec_reset(xc, &token);
		break;
	case JBXVT_TOKEN_RI: // Reverse index
		LOG("JBXVT_TOKEN_RI");
		jbxvt_index_from(xc, -n, jbxvt_get_screen()->margin.t);
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
		jbxvt_scroll_to(xc, t[0]);
		break;
	case JBXVT_TOKEN_SC:
		LOG("JBXVT_TOKEN_SC");
		jbxvt_save_cursor();
		break;
	case JBXVT_TOKEN_SCS0: //  SCS G0
		LOG("JBXVT_TOKEN_SCS0");
		select_charset(t[0], 0);
		break;
	case JBXVT_TOKEN_SCS1: //  SCS G1
		LOG("JBXVT_TOKEN_SCS1");
		select_charset(t[0], 1);
		break;
	case JBXVT_TOKEN_SD:
		LOG("JBXVT_TOKEN_SD");
		// scroll down n lines
		t[0] = - t[0]; // fall through
	case JBXVT_TOKEN_SU:
		LOG("JBXVT_TOKEN_SU");
		// scroll up n lines;
		LOG("JBXVT_TOKEN_SU");
		scroll(xc, jbxvt_get_screen()->margin.top,
			jbxvt_get_screen()->margin.bot, t[0]);
		break;
	case JBXVT_TOKEN_SELINSRT:
		LOG("JBXVT_TOKEN_SELINSRT");
		jbxvt_request_selection(xc, t[0]);
		break;
	case JBXVT_TOKEN_SPA: // start protected area
		LOG("FIXME JBXVT_TOKEN_SPA");
		break;
	case JBXVT_TOKEN_SS2:
		LOG("FIXME JBXVT_TOKEN_SS2");
		break;
	case JBXVT_TOKEN_SS3:
		LOG("FIXME JBXVT_TOKEN_SS3");
		break;
	case JBXVT_TOKEN_STRING: // don't log
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
		jbxvt_get_screen()->decpm = false;
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
		jbxvt_move(xc, 0, t[0] - 1, JBXVT_COLUMN_RELATIVE);
		break;
	case JBXVT_TOKEN_VPR: // vertical position relative
		LOG("JBXVT_TOKEN_VPR");
		jbxvt_move(xc, 0, t[0] - 1,
			JBXVT_COLUMN_RELATIVE | JBXVT_ROW_RELAATIVE);
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
