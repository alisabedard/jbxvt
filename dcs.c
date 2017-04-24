/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "dcs.h"
#include "JBXVTToken.h"
#include "JBXVTTokenType.h"
#include "cmdtok.h"
#include "libjb/log.h"
static void check_st(xcb_connection_t * xc, struct JBXVTToken * t)
{
	int_fast16_t c = jbxvt_pop_char(xc, 0);
	if (c != JBXVT_TOKEN_ST)
		t->type = JBXVT_TOKEN_NULL;
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
				t->type = JBXVT_TOKEN_QUERY_SCA;
				check_st(xc, t);
				break;
			case 'q':
				t->type = JBXVT_TOKEN_QUERY_SCL;
				check_st(xc, t);
				break;
			}
		case 'm':
			t->type = JBXVT_TOKEN_QUERY_SLRM;
			check_st(xc, t);
			break;
		case 'r':
			t->type = JBXVT_TOKEN_QUERY_STBM;
			check_st(xc, t);
			break;
		case 's':
			t->type = JBXVT_TOKEN_QUERY_SLRM;
			check_st(xc, t);
			break;
		case ' ':
			c = jbxvt_pop_char(xc, 0);
			if (c != 'q')
				return;
			t->type = JBXVT_TOKEN_QUERY_SCUSR;
			check_st(xc, t);
			break;
		}
		break;
	case '+':
		c = jbxvt_pop_char(xc, 0);
		switch (c) {
		case 'p':
		case 'q':
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
