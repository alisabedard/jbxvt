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
	int_fast16_t c = jbxvt_pop_char(0);
	if (c != JBXVT_TOKEN_ST)
		t->type = JBXVT_TOKEN_NULL;
}
void jbxvt_dcs(struct Token * t)
{
	assert(t);
	int_fast16_t c = jbxvt_pop_char(0);
	switch (c) {
	case '0':
	case '1':
		LOG("FIXME: User defined keys are unimplemented.");
		return;
	case '$':
		c = jbxvt_pop_char(0);
		if (c != 'q')
			return;
		// RQSS:  Request status string
		c = jbxvt_pop_char(0); // next char
		switch (c) {
		case '"':
			c = jbxvt_pop_char(0); // next
			switch (c) {
#define CASE_Q(ch, tk) case ch:t->type=JBXVT_TOKEN_QUERY_##tk;check_st(t);break;
			CASE_Q('p', SCA);
			CASE_Q('q', SCL);
			}
			break;
		CASE_Q('m', SLRM);
		CASE_Q('r', STBM);
		CASE_Q('s', SLRM);
		case ' ':
			c = jbxvt_pop_char(0);
			if (c != 'q')
				return;
			t->type = JBXVT_TOKEN_QUERY_SCUSR;
			check_st(t);
			break;
		}
		break;
	case '+':
		c = jbxvt_pop_char(0);
		switch (c) {
		case 'p':
		case 'q':
			LOG("FIXME: termcap support unimplemented");
		}
		break;
	case 0x1b:
		jbxvt_pop_char(0);
		jbxvt_pop_char(0);
		jbxvt_push_char('-');
		break;
	case 0xd0:
		jbxvt_push_char('?');
		break;
	default:
		LOG("Unhandled DCS, starting with 0x%x", (int)c);
	}
}
