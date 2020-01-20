/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "dcs.h"
#include "JBXVTToken.h"
#include "cmdtok.h"
#include "libjb/log.h"
static void check_st(xcb_connection_t * xc, struct JBXVTToken * t,
	const enum JBXVTTokenIndex new_type)
{
	int_fast16_t c = jbxvt_pop_char(xc, 0);
	if (c != JBXVT_TOKEN_ST)
		t->type = JBXVT_TOKEN_NULL;
	else
		t->type = new_type;
}
void jbxvt_dcs(xcb_connection_t * xc, struct JBXVTToken * t)
{
	int_fast16_t c = jbxvt_pop_char(xc, 0);
	switch (c) {
	case '0':
	case '1':
		LOG("FIXME: User defined keys are unimplemented.");
		return;
	case '$':
		c = jbxvt_pop_char(xc, 0);
		if (c != 'q')
			return;
		// RQSS:  Request status string
		c = jbxvt_pop_char(xc, 0); // next char
		switch (c) {
		case '"':
			c = jbxvt_pop_char(xc, 0); // next
			switch (c) {
			case 'p':
				check_st(xc, t, JBXVT_TOKEN_QUERY_SCA);
				break;
			case 'q':
				check_st(xc, t, JBXVT_TOKEN_QUERY_SCL);
				break;
			}
                        // FALL THROUGH
		case 'm':
			check_st(xc, t, JBXVT_TOKEN_QUERY_SGR);
			break;
		case 'r':
			check_st(xc, t, JBXVT_TOKEN_QUERY_STBM);
			break;
		case 's':
			check_st(xc, t, JBXVT_TOKEN_QUERY_SLRM);
			break;
		case ' ':
			c = jbxvt_pop_char(xc, 0);
			if (c != 'q')
				return;
			check_st(xc, t, JBXVT_TOKEN_QUERY_SCUSR);
			break;
		}
		break;
	case '+': // Set/request termcap/terminfo data
		c = jbxvt_pop_char(xc, 0);
		switch (c) {
		case 'p': // Set termcap/terminfo data
		case 'q': // Request termcap/terminfo data
			LOG("FIXME: termcap support unimplemented");
		}
		break;
	case 0x1b:
		jbxvt_pop_char(xc, 0);
		jbxvt_pop_char(xc, 0);
		jbxvt_push_char('-');
		break;
	case 0xd0:
		jbxvt_push_char('?');
		break;
	default:
		LOG("Unhandled DCS, starting with 0x%x", (int)c);
	}
}
