/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "esc.h"
#include "command.h"
#include "cmdtok.h"
#include "dcs.h"
#include "jbxvt.h"
#include "screen.h"
void jbxvt_csi(int_fast16_t c, struct Token * restrict tk)
{
	c = jbxvt_pop_char(0);
	if (c >= '<' && c <= '?') {
		tk->private = c;
		c = jbxvt_pop_char(0);
	}
	//  read any numerical arguments
	uint_fast16_t i = 0;
	do {
		uint_fast16_t n = 0;
		while (c >= '0' && c <= '9') { // is a number
			// Advance position and convert
			n = n * 10 + c - '0';
			c = jbxvt_pop_char(0); // next digit
		}
		if (i < JBXVT_TOKEN_MAX_ARGS)
			  tk->arg[i++] = n;
		if (c == JBXVT_TOKEN_ESC)
			  jbxvt_push_char(c);
		if (c < ' ')
			  return;
		if (c < '@')
			  c = jbxvt_pop_char(0);
	} while (c < '@' && c >= ' ');
	if (c == JBXVT_TOKEN_ESC)
		  jbxvt_push_char(c);
	tk->nargs = i;
	tk->type = c;
}
void jbxvt_end_cs(int_fast16_t c, struct Token * restrict tk)
{
	c = jbxvt_pop_char(0);
	uint_fast16_t n = 0;
	while (c >= '0' && c <= '9') {
		n = n * 10 + c - '0';
		c = jbxvt_pop_char(0);
	}
	tk->arg[0] = n;
	tk->nargs = 1;
	c = jbxvt_pop_char(0);
	register uint_fast16_t i = 0;
	while ((c & 0177) >= ' ' && i < TKS_MAX) {
		if (c >= ' ')
			  tk->string[i++] = c;
		c = jbxvt_pop_char(0);
	}
	tk->length = i;
	tk->string[i] = 0;
	tk->type = JBXVT_TOKEN_TXTPAR;
}
void jbxvt_esc(int_fast16_t c, struct Token * restrict tk)
{
	c = jbxvt_pop_char(0);
	switch(c) {
	case '[': // CSI
		jbxvt_csi(c, tk);
		break;
	case ']': // OSC
		jbxvt_end_cs(c, tk);
		break;
	case ' ':
		c = jbxvt_pop_char(0);
		switch (c) {
#define CASE_A(ch, tok, a) case ch: tk->type = tok, tk->arg[0] = a;\
			tk->nargs=1; break;
#define CASE_T(ch, tok) case ch: tk->type = tok; break;
#define CASE_M(ch, mod, v) case ch: jbxvt.mode.mod = v; break;
		CASE_T('F', JBXVT_TOKEN_S7C1T);
		CASE_T('G', JBXVT_TOKEN_S8C1T);
		CASE_T('L', JBXVT_TOKEN_ANSI1);
		CASE_T('M', JBXVT_TOKEN_ANSI2);
		CASE_T('N', JBXVT_TOKEN_ANSI3);
		}
		break;
	case '#':
		c = jbxvt_pop_char(0);
		switch(c) {
		CASE_T('3', JBXVT_TOKEN_DHLT);
		CASE_T('4', JBXVT_TOKEN_DHLB);
		CASE_T('5', JBXVT_TOKEN_SWL);
		CASE_T('6', JBXVT_TOKEN_DWL);
		CASE_T('8', JBXVT_TOKEN_ALN);
		}
		break;
	case '(': // G0 charset
	CASE_A(')', c, jbxvt_pop_char(0));
	case '%': // UTF charset switch
		c = jbxvt_pop_char(0);
		switch (c) {
		CASE_T('@', JBXVT_TOKEN_CS_DEF);
		CASE_T('G', JBXVT_TOKEN_CS_UTF8);
		}
		break;
	CASE_T('6', JBXVT_TOKEN_RI); // BI: back index
	CASE_T('9', JBXVT_TOKEN_IND); // FI: forward index
	case '7': // SC: save cursor
	case '8': // RC: restore cursor
	case '=': // PAM: keypad to application mode
	case '>': // PNM: keypad to numeric mode
		tk->type = c;
		break;
	CASE_T('^', JBXVT_TOKEN_PM); // PM: Privacy message (ended by ESC \)
	CASE_T('\\', JBXVT_TOKEN_ST);
	CASE_M('<', decanm, true); // exit vt52 mode
	case 'A': // vt52 cursor up
	case 'B': // vt52 cursor down
	case 'C': // vt52 cursor left
		tk->type = c;
		break;
	CASE_T('D', jbxvt.mode.decanm ? JBXVT_TOKEN_IND : JBXVT_TOKEN_CUF);
	CASE_T('c', JBXVT_TOKEN_RIS); // Reset to Initial State
	CASE_M('e', dectcem, true); // enable cursor (vt52 GEMDOS)
	CASE_M('f', dectcem, false); // disable cursor (vt52 GEMDOS)
	CASE_T('E', JBXVT_TOKEN_NEL);
	CASE_T('F', JBXVT_TOKEN_ENTGM52); // Enter VT52 graphics mode
	CASE_T('G', JBXVT_TOKEN_EXTGM52); // Leave VT52 graphics mode
	CASE_T('H', jbxvt.mode.decanm ? JBXVT_TOKEN_HTS : JBXVT_TOKEN_HOME);
	CASE_A('l', jbxvt.mode.decanm ? JBXVT_TOKEN_MEMLOCK : JBXVT_TOKEN_EL, 2);
	case 'I':
		jbxvt_index_from(-1, jbxvt.scr.current->cursor.y);
		tk->type = JBXVT_TOKEN_CUU;
		break;
	CASE_A('J', JBXVT_TOKEN_EL, 1); // vt52 erase to end of line
	CASE_T('j', JBXVT_TOKEN_SC); // save cursor (vt52g)
	CASE_T('K', JBXVT_TOKEN_ED); // vt42 erase to end of screen
	CASE_T('k', JBXVT_TOKEN_RC); // restore cursor (vt52g)
	CASE_T('L', JBXVT_TOKEN_IL); // insert line (vt52)
	CASE_T('M', jbxvt.mode.decanm ? JBXVT_TOKEN_RI : JBXVT_TOKEN_DL);
	CASE_T('N', JBXVT_TOKEN_SS2);
	CASE_T('O', JBXVT_TOKEN_SS3);
	CASE_A('o', JBXVT_TOKEN_EL, 1); // clear to start of line (vt52g)
	case 'P':
		jbxvt_dcs(tk);
		break;
	CASE_M('p', decscnm, true); // reverse video mode (vt52g)
	CASE_M('q', decscnm, false); // normal video (vt52g)
	CASE_T('V', JBXVT_TOKEN_SPA);
	CASE_M('v', decawm, true); // wrap on
	CASE_T('W', JBXVT_TOKEN_EPA);
	CASE_M('w', decawm, false); // wrap off
	CASE_T('X', JBXVT_TOKEN_SOS);
	case 'Y':
		tk->type = JBXVT_TOKEN_CUP;
		// -32 to decode, + 1 to be vt100 compatible
		tk->arg[1] = jbxvt_pop_char(0) - 31;
		tk->arg[0] = jbxvt_pop_char(0) - 31;
		tk->nargs = 2;
	case 'Z':
		if (jbxvt.mode.decanm) // vt100+ mode
			tk->type = JBXVT_TOKEN_ID;
		else // I am a VT52
			cprintf("\033/Z");
		break;
	}
}
