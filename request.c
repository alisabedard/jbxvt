/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "request.h"
#include <stdio.h>
#include "JBXVTToken.h"
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "libjb/log.h"
// Load LEDs, SCUSR and SCA
void jbxvt_handle_JBXVT_TOKEN_LL(void * xc __attribute__((unused)),
	struct JBXVTToken * token)
{
	int16_t * restrict t = token->arg;
	LOG("t[0]: %d, t[1]: %d", t[0], t[1]);
	switch (t[1]) {
	case 0:
	case ' ': // SCUSR
		LOG("SCUSR");
		jbxvt_set_cursor_attr(t[0]);
		break;
	case '"': // SCA - Select Character protection Attribute
		switch(t[0]) {
		default:
		case 0:
		case 2: // DECSED and DECSEL can erase
			LOG("SCA can erase");
			break;
		case 1: // DECSED and DECSEL cannot erase
			LOG("SCA cannot erase");
			break;
		}
		break;
	default: // LL
		switch (t[0]) {
		default:
		case 0:
			LOG("LL clear all LEDs");
			break;
		case 1:
			LOG("LL light num lock");
			break;
		case 2:
			LOG("LL light caps lock");
			break;
		case 3:
			LOG("LL light scroll lock");
			break;
		case 21:
			LOG("LL extinguish num lock");
			break;
		case 22:
			LOG("LL extinguish caps lock");
			break;
		case 23:
			LOG("LL extinguish scroll lock");
			break;
		}
		LOG("LL -- unimplemented");
	}

}
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
