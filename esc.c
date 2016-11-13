/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.*/
#include "esc.h"
#include "command.h"
#include "cmdtok.h"
#include "dcs.h"
#include "mode.h"
#include "screen.h"
#include <stdio.h>
void jbxvt_csi(xcb_connection_t * xc,
	int_fast16_t c, struct JBXVTToken * restrict tk)
{
	c = jbxvt_pop_char(xc, 0);
	if (c >= '<' && c <= '?') {
		tk->private = c;
		c = jbxvt_pop_char(xc, 0);
	}
	//  read any numerical arguments
	uint_fast16_t i = 0;
	do {
		{ // n scope
			uint_fast16_t n = 0;
			while (c >= '0' && c <= '9') { // is a number
				// Advance position and convert
				n = n * 10 + c - '0';
				c = jbxvt_pop_char(xc, 0); // next digit
			}
			if (i < JBXVT_TOKEN_MAX_ARGS)
				  tk->arg[i++] = n;
		}
		if (c == JBXVT_TOKEN_ESC)
			  jbxvt_push_char(c);
		if (c < ' ')
			  return;
		if (c < '@')
			  c = jbxvt_pop_char(xc, 0);
	} while (c < '@' && c >= ' ');
	if (c == JBXVT_TOKEN_ESC)
		  jbxvt_push_char(c);
	tk->nargs = i;
	tk->type = c;
}
void jbxvt_end_cs(xcb_connection_t * xc,
	int_fast16_t c, struct JBXVTToken * restrict tk)
{
	c = jbxvt_pop_char(xc, 0);
	uint_fast16_t n = 0;
	while (c >= '0' && c <= '9') {
		n = n * 10 + c - '0';
		c = jbxvt_pop_char(xc, 0);
	}
	tk->arg[0] = n;
	tk->nargs = 1;
	c = jbxvt_pop_char(xc, 0);
	register uint_fast16_t i = 0;
	while ((c & 0177) >= ' ' && i < JBXVT_TOKEN_MAX_LENGTH) {
		if (c >= ' ')
			  tk->string[i++] = c;
		c = jbxvt_pop_char(xc, 0);
	}
	tk->length = i;
	tk->string[i] = 0;
	tk->type = JBXVT_TOKEN_TXTPAR;
}
void jbxvt_esc(xcb_connection_t * xc,
	int_fast16_t c, struct JBXVTToken * restrict tk)
{
	c = jbxvt_pop_char(xc, 0);
	switch(c) {
	case '[': // CSI
		jbxvt_csi(xc, c, tk);
		break;
	case ']': // OSC
		jbxvt_end_cs(xc, c, tk);
		break;
	case ' ':
		c = jbxvt_pop_char(xc, 0);
		switch (c) {
		case 'F':
			tk->type = JBXVT_TOKEN_S7C1T;
			break;
		case 'G':
			tk->type = JBXVT_TOKEN_S8C1T;
			break;
		case 'L':
			tk->type = JBXVT_TOKEN_ANSI1;
			break;
		case 'M':
			tk->type = JBXVT_TOKEN_ANSI2;
			break;
		case 'N':
			tk->type = JBXVT_TOKEN_ANSI3;
			break;
		}
		break;
	case '#':
		c = jbxvt_pop_char(xc, 0);
		switch(c) {
		case '3':
			tk->type = JBXVT_TOKEN_DHLT;
			break;
		case '4':
			tk->type = JBXVT_TOKEN_DHLB;
			break;
		case '5':
			tk->type = JBXVT_TOKEN_SWL;
			break;
		case '6':
			tk->type = JBXVT_TOKEN_DWL;
			break;
		case '8':
			tk->type = JBXVT_TOKEN_ALN;
			break;
		}
		break;
	case '(': // G0 character set
	case ')': // G1
	case '*': // G2
	case '+': // G3
	case '-': // G1
	case '.': // G2
	case '/': // G3
		tk->type = c;
		tk->arg[0] = jbxvt_pop_char(xc, 0);
		tk->nargs = 1;
		break;
	case '%': // UTF charset switch
		c = jbxvt_pop_char(xc, 0);
		switch (c) {
		case '@':
			tk->type = JBXVT_TOKEN_CS_DEF;
			break;
		case 'G':
			tk->type = JBXVT_TOKEN_CS_UTF8;
			break;
		}
		break;
	case '6': // BI: back index
		tk->type = JBXVT_TOKEN_RI;
		break;
	case '9': // FI: forward index
		tk->type = JBXVT_TOKEN_IND;
		break;
	case '7': // SC: save cursor
	case '8': // RC: restore cursor
	case '=': // PAM: keypad to application mode
	case '>': // PNM: keypad to numeric mode
		tk->type = c;
		break;
	case '^': // PM: Privacy message (ended by ESC \)
		tk->type = JBXVT_TOKEN_PM;
		break;
	case '\\': // string terminator
		tk->type = JBXVT_TOKEN_ST;
		break;
	case '<': // exit vt52 mode
		jbxvt_get_modes()->decanm = true;
		break;
	case 'A': // vt52 cursor up
	case 'B': // vt52 cursor down
	case 'C': // vt52 cursor left
		tk->type = c;
		break;
	case 'D':
		tk->type = jbxvt_get_modes()->decanm
			? JBXVT_TOKEN_IND : JBXVT_TOKEN_CUF;
		break;
	case 'c':
		tk->type = JBXVT_TOKEN_RIS;
		break; // Reset to Initial State
	case 'e': // enable cursor (vt52)
		jbxvt_get_modes()->dectcem = true;
		break;
	case 'f': // disable cursor (vt52)
		jbxvt_get_modes()->dectcem = true;
		break;
	case 'E':
		tk->type = JBXVT_TOKEN_NEL;
		break;
	case 'F': // Enter VT52 graphics mode
		tk->type = JBXVT_TOKEN_ENTGM52;
		break;
	case 'G': // Leave VT52 graphics mode
		tk->type = JBXVT_TOKEN_EXTGM52;
		break;
	case 'H':
		tk->type = jbxvt_get_modes()->decanm
			? JBXVT_TOKEN_HTS : JBXVT_TOKEN_HOME;
		break;
	case 'l':
		tk->type = jbxvt_get_modes()->decanm
			? JBXVT_TOKEN_MEMLOCK : JBXVT_TOKEN_EL;
		tk->arg[0] = 2;
		tk->nargs = 1;
		break;
	case 'I':
		jbxvt_index_from(xc, -1, jbxvt_get_screen()->cursor.y);
		tk->type = JBXVT_TOKEN_CUU;
		break;
	case 'J': // erase to end of line
		tk->type = JBXVT_TOKEN_EL;
		tk->arg[0] = 1;
		tk->nargs = 1;
		break;
	case 'j': // save cursor
		tk->type = JBXVT_TOKEN_SC;
		break;
	case 'K': // erase to end of screen
		tk->type = JBXVT_TOKEN_ED;
		break;
	case 'k': // restore cursor
		tk->type = JBXVT_TOKEN_RC;
		break;
	case 'L': // insert line
		tk->type = JBXVT_TOKEN_IL;
		break;
	case 'M': // reverse index or delete line
		tk->type = jbxvt_get_modes()->decanm
			? JBXVT_TOKEN_RI : JBXVT_TOKEN_DL;
		break;
	case 'N':
		tk->type = JBXVT_TOKEN_SS2;
		break;
	case 'O':
		tk->type = JBXVT_TOKEN_SS3;
		break;
	case 'o': // clear to start of line
		tk->type = JBXVT_TOKEN_EL;
		tk->arg[0] = 1;
		tk->nargs = 1;
		break;
	case 'P': // device control string
		jbxvt_dcs(xc, tk);
		break;
	case 'p': // reverse video mode
		jbxvt_get_modes()->decscnm = true;
		break;
	case 'q': // normal video mode
		jbxvt_get_modes()->decscnm = false;
		break;
	case 'V': // start protected area
		tk->type = JBXVT_TOKEN_SPA;
		break;
	case 'v': // auto wrap mode on
		jbxvt_get_modes()->decawm = true;
		break;
	case 'W': // end protected area
		tk->type = JBXVT_TOKEN_EPA;
		break;
	case 'w': // auto wrap mode off
		jbxvt_get_modes()->decawm = false;
		break;
	case 'X': // start of string
		tk->type = JBXVT_TOKEN_SOS;
		break;
	case 'Y':
		tk->type = JBXVT_TOKEN_CUP;
		// -32 to decode, + 1 to be vt100 compatible
		tk->arg[1] = jbxvt_pop_char(xc, 0) - 31;
		tk->arg[0] = jbxvt_pop_char(xc, 0) - 31;
		tk->nargs = 2;
	case 'Z':
		if (jbxvt_get_modes()->decanm) // vt100+ mode
			tk->type = JBXVT_TOKEN_ID;
		else // I am a VT52
			dprintf(jbxvt_get_fd(), "\033/Z");
		break;
	}
}
