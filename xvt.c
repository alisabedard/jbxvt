/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#define LOG_LEVEL 3
#if LOG_LEVEL == 0
#undef DEBUG
#endif//LOG_LEVEL
#include "xvt.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
#include "JBXVTToken.h"
#include "JBXVTTokenType.h"
#include "cases.h"
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "dec_reset.h"
#include "double.h"
#include "dsr.h"
#include "edit.h"
#include "erase.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "mode.h"
#include "mc.h"
#include "move.h"
#include "request.h"
#include "sbar.h"
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
// Return value sanitized for tokens with optional arguments, defaulting to 1
static int16_t get_arg(struct JBXVTToken * t)
{
	return t->nargs > 0 && t->arg[0] ? t->arg[0] : 1;
}
HANDLE(ALN) // screen alignment test
{
	NOPARM_TOKEN();
	jbxvt_efill(xc);
}
HANDLE(CHA) // cursor character absolute column
{
	jbxvt_move(xc, get_0(token->arg[0]), 0, JBXVT_ROW_RELATIVE);
}
HANDLE(CHAR)
{
	jbxvt_handle_tk_char(xc, token->tk_char);
}
HANDLE(CHT) // change tab stop
{
	jbxvt_cht(xc, get_arg(token));
}
HANDLE(CPL) // cursor previous line
{
	jbxvt_move(xc, 0, -get_arg(token), 0);
}
HANDLE(CNL) // cursor next line
{
	jbxvt_move(xc, 0, get_arg(token), 0);
}
HANDLE(CS_G0)
{
	NOPARM_XC();
	select_charset(token->arg[0], 0);
}
HANDLE(CS_G1)
{
	NOPARM_XC();
	select_charset(token->arg[0], 1);
}
ALIAS(CS_ALT_G1, CS_G1);
HANDLE(CS_G2)
{
	NOPARM_XC();
	select_charset(token->arg[0], 2);
}
ALIAS(CS_ALT_G2, CS_G2);
HANDLE(CS_G3)
{
	NOPARM_XC();
	select_charset(token->arg[0], 3);
}
ALIAS(CS_ALT_G3, CS_G3);
HANDLE(CUB) // cursor back
{
	jbxvt_move(xc, -get_arg(token), 0, JBXVT_ROW_RELATIVE
		| JBXVT_COLUMN_RELATIVE);
}
HANDLE(CUD) // cursor down
{
	jbxvt_move(xc, 0, get_arg(token), JBXVT_ROW_RELATIVE
		| JBXVT_COLUMN_RELATIVE);
}
HANDLE(CUF) // cursor forward
{
	jbxvt_move(xc, get_arg(token), 0, JBXVT_ROW_RELATIVE
		| JBXVT_COLUMN_RELATIVE);
}
HANDLE(CUP) // cursor position (absolute)
{
	cup(xc, token->arg);
}
ALIAS(HVP, CUP); // horizontal vertical position
HANDLE(CUU) // cursor up
{
	jbxvt_move(xc, 0, -get_arg(token), JBXVT_ROW_RELATIVE
		| JBXVT_COLUMN_RELATIVE);
}
HANDLE(DL) // delete line
{
	jbxvt_index_from(xc, get_arg(token), jbxvt_get_y());
}
HANDLE(DSR) // device status report
{
	NOPARM_XC();
	jbxvt_handle_dsr(token->arg[0]);
}
HANDLE(DWL) // double width line
{
	NOPARM_TOKEN();
	jbxvt_set_double_width_line(xc, true);
}
HANDLE(ED) // erase display
{
	jbxvt_erase_screen(xc, token->arg[0]); // don't use get_arg()
}
HANDLE(EL) // erase line
{
	jbxvt_erase_line(xc, token->arg[0]); // don't use get_arg()
}
HANDLE(DA) // DECID
{
	NOPARM();
	decid();
}
ALIAS(ID, DA); // DECID
HANDLE(DCH) // delete character
{
	jbxvt_edit_characters(xc, get_arg(token), true);
}
ALIAS(ECH, DCH); // erase character
HANDLE(ENTGM52) // enter vt52 graphics mode
{
	NOPARM();
	gm52(true);
}
HANDLE(EXTGM52) // exit vt52 graphics mode
{
	NOPARM();
	gm52(false);
}
HANDLE(HOME)
{
	(void)token;
	jbxvt_set_scroll(xc, 0);
	jbxvt_move(xc, 0, 0, 0);
}
HANDLE(HPA) // horizontal position absolute
{
	jbxvt_move(xc, get_0(token->arg[0]), 0, JBXVT_ROW_RELATIVE);
}
HANDLE(HPR) // horizontal position relative
{
	jbxvt_move(xc, get_0(token->arg[0]), 0, JBXVT_COLUMN_RELATIVE
		| JBXVT_ROW_RELATIVE);
}
HANDLE(HTS) // horizontal tab stop
{
	NOPARM();
	jbxvt_set_tab(jbxvt_get_x(), true);
}
HANDLE(ICH) // insert blank characters
{
	jbxvt_edit_characters(xc, get_arg(token), false);
}
HANDLE(IL) // insert line
{
	jbxvt_index_from(xc, -get_arg(token), jbxvt_get_y());
}
HANDLE(IND) // index (scroll)
{
	jbxvt_index_from(xc, get_arg(token), jbxvt_get_margin()->t);
}
HANDLE(NEL) // next line (move to the first position on the next line)
{
	NOPARM_TOKEN();
	jbxvt_move(xc, 0, jbxvt_get_y() + 1, 0);
}
HANDLE(PAM) // application mode keys
{
	NOPARM();
	jbxvt_set_keys(true, false);
}
HANDLE(PM) // privacy message
{
	NOPARM();
	jbxvt_get_current_screen()->decpm = true;
}
HANDLE(PNM) // numeric key mode
{
	NOPARM();
	jbxvt_set_keys(false, false);
}
HANDLE(RC)
{
	NOPARM_TOKEN();
	jbxvt_restore_cursor(xc);
}
EXTERN_ALIAS(RESET, jbxvt_dec_reset);
ALIAS(SET, RESET);
HANDLE(RI) // reverse index
{
	jbxvt_index_from(xc, -get_arg(token), jbxvt_get_margin()->top);
}
HANDLE(RIS) // reset to initial state
{
	NOPARM_TOKEN();
	jbxvt_get_modes()->dectcem = true;
	jbxvt_reset(xc);
}
HANDLE(RQM)
{
	(void)xc;
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
static void set_s8c1t(const bool val)
{
	jbxvt_get_modes()->s8c1t = val;
}
HANDLE(S7C1T) // 7-bit control sequences
{
	NOPARM();
	set_s8c1t(false);
}
HANDLE(S8C1T) // 8-bit control sequences
{
	NOPARM();
	set_s8c1t(true);
}
HANDLE(SAVEPM) // save private modes
{
	NOPARM();
	jbxvt_save_modes();
}
HANDLE(SBGOTO) // Scroll to where token arg[0] is at top of screen.
{
	jbxvt_scroll_to(xc, token->arg[0]);
}
HANDLE(SBSWITCH)
{
	NOPARM_TOKEN();
	jbxvt_toggle_scrollbar(xc);
}
HANDLE(SC)
{
	NOPARM();
	jbxvt_save_cursor();
}
HANDLE(SD) // scroll down
{
	handle_scroll(xc, -token->arg[0]);
}
HANDLE(SELINSRT)
{
	jbxvt_request_selection(xc, token->arg[0]);
}
EXTERN_ALIAS(SGR, jbxvt_handle_sgr);
HANDLE(SS2)
{
	NOPARM();
	jbxvt_get_modes()->ss2 = true;
}
HANDLE(SS3)
{
	NOPARM();
	jbxvt_get_modes()->ss3 = true;
}
HANDLE(ST) // string terminator
{
	NOPARM();
	jbxvt_get_current_screen()->decpm = false;
}
HANDLE(STBM) // set top and bottom margins
{
	NOPARM_XC();
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
HANDLE(STRING)
{
	jbxvt_string(xc, token->string, token->length, token->nlcount);
}
HANDLE(SU) // scroll up
{
	handle_scroll(xc, token->arg[0]);
}
HANDLE(SWL) // single width line
{
	NOPARM_TOKEN();
	jbxvt_set_double_width_line(xc, false);
}
HANDLE(TBC) // tabulation clear
{
	NOPARM_XC();
	const uint8_t t = token->arg[0];
	/* Note:  Attempting to simplify this results
	   in vttest test failure.  */
	if (t == 3)
		jbxvt_set_tab(-1, false);
	else if (!t)
		jbxvt_set_tab(jbxvt_get_x(), false);

}
HANDLE(VPA) // vertical position absolute
{
	vp(xc, token->arg[0], false);
}
HANDLE(VPR) // vertical position relative
{
	vp(xc, token->arg[0], true);
}
bool jbxvt_parse_token(xcb_connection_t * xc)
{
	struct JBXVTToken token;
	jbxvt_get_token(xc, &token);
	switch (token.type) {
	case JBXVT_TOKEN_EOF:
		LOG("JBXVT_TOKEN_EOF");
		return false;
#include "cases.c"
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
