// Copyright 2016, Jeffrey E. Bedard
#include "dsr.h"
#include "cmdtok.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include <stdio.h>
#include <stdlib.h>
void jbxvt_handle_dsr(const int16_t arg)
{
	LOG("handle_dsr(%d)", arg);
	switch (arg) {
	case 6 : {
		const struct JBDim c = jbxvt.scr.current->cursor;
		dprintf(jbxvt.com.fd, "%s%d;%dR", jbxvt_get_csi(),
			c.y + 1, c.x + 1);
		break;
	}
	case 7 :
		//  Send the name of the display to the command.
		dprintf(jbxvt.com.fd, "%s\r", getenv("DISPLAY"));
		break;
	case 5: // command from host requesting status
		// 0 is response for 'Ready, no malfunctions'
	case 15: // Test printer status
	case 25: // Test UDK status?
	case 26: // Test keyboard status
	default:
		dprintf(jbxvt.com.fd, "%s0n", jbxvt_get_csi());
	}
}

