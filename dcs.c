/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "dcs.h"

#include "command.h"
#include "cmdtok.h"
#include "jbxvt.h"
#include "libjb/log.h"

#include <assert.h>

static void check_st(struct Token * t)
{
	assert(t);
	int_fast16_t c = get_com_char(0);
	if (c != TK_ST)
		t->type = TK_NULL;
}

void jbxvt_dcs(struct Token * t)
{
	assert(t);
	int_fast16_t c = get_com_char(0);
	switch (c) {
	case '0':
	case '1':
		LOG("FIXME: User defined keys are unimplemented.");
		return;
	case '$':
		c = get_com_char(0);
		if (c != 'q')
			return;
		// RQSS:  Request status string
		c = get_com_char(0); // next char
		switch (c) {
		case '"':
			c = get_com_char(0); // next
			switch (c) {
#define CASE_Q(ch, tk) case ch:t->type=TK_QUERY_##tk;check_st(t);break;
			CASE_Q('p', SCA);
			CASE_Q('q', SCL);
			}
			break;
		CASE_Q('m', SLRM);
		CASE_Q('r', STBM);
		CASE_Q('s', SLRM);
		case ' ':
			c = get_com_char(0);
			if (c != 'q')
				return;
			t->type = TK_QUERY_SCUSR;
			check_st(t);
			break;
		}
		break;
	case '+':
		c = get_com_char(0);
		switch (c) {
		case 'p':
		case 'q':
			LOG("FIXME: termcap support unimplemented");
		}
		break;
	case 0x1b:
		get_com_char(0);
		get_com_char(0);
		put_com_char('-');
		break;
	case 0xd0:
		put_com_char('?');
		break;
	default:
		LOG("Unhandled DCS, starting with 0x%x", (int)c);
	}
}
