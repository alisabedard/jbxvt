/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "request.h"
#include <stdio.h>
#include "JBXVTToken.h"
#include "cmdtok.h"
#include "command.h"
#include "libjb/log.h"
void jbxvt_handle_JBXVT_TOKEN_REQTPARAM(void * xc __attribute__((unused)),
	struct JBXVTToken * token)
{
	const uint8_t t = token->arg[0];
	// Send REPTPARAM
	const uint8_t sol = t + 2, par = 1, nbits = 1,
	      flags = 0, clkmul = 1;
	const uint16_t xspeed = 88, rspeed = 88;
	dprintf(jbxvt_get_fd(), "%s[%d;%d;%d;%d;%d;%d;%dx", jbxvt_get_csi(),
		sol, par, nbits, xspeed, rspeed, clkmul, flags);
	LOG("ESC[%d;%d;%d;%d;%d;%d;%dx", sol, par, nbits,
		xspeed, rspeed, clkmul, flags);
}
