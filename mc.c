/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "mc.h"
#include <stdint.h>
#include <stdio.h>
#include "JBXVTToken.h"
#include "command.h"
#include "cursor.h"
#include "libjb/log.h"
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
		dprintf(jbxvt_get_fd(), "%d", jbxvt_get_y() + 1);
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
void jbxvt_handle_JBXVT_TOKEN_MC(void * xc __attribute__((unused)),
	struct JBXVTToken * token)
{
	int16_t * restrict t = token->arg;
	if (token->private == '?')
		handle_token_mc_public(t);
	else
		handle_token_mc_private(t);
}
