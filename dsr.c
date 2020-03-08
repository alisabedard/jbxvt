// Copyright 2017, Jeffrey E. Bedard
#include "dsr.h"
#include <stdio.h> // for dprintf
#include <stdlib.h> // for getenv
#include "command.h"
#include "cursor.h"
#include "libjb/log.h"
#include "mode.h"
void jbxvt_handle_dsr(const int16_t arg)
{
    LOG("handle_dsr(%d)", arg);
    switch (arg) {
    case 6 :
        LOG("CPR: cursor position report");
        dprintf(jbxvt_get_fd(), "%s%d;%dR", jbxvt_get_csi(),
            jbxvt_get_y() + 1, jbxvt_get_y() + 1);
        break;
    case 7 :
        //  Send the name of the display to the command.
        dprintf(jbxvt_get_fd(), "%s\r", getenv("DISPLAY"));
        break;
    case 15: // Test printer status
        // no printer
        dprintf(jbxvt_get_fd(), "%s?13n", jbxvt_get_csi());
        break;
    case 25: // Test UDK status, user defined keys
        // user defined keys locked
        dprintf(jbxvt_get_fd(), "%s?21n", jbxvt_get_csi());
        break;
    case 26: // Test keyboard status
        // North American keyboard
        dprintf(jbxvt_get_fd(), "%s?27;1n", jbxvt_get_csi());
        break;
    case 5: // command from host requesting status
        // 0 is response for 'Ready, no malfunctions'
    default:
        dprintf(jbxvt_get_fd(), "%s0n", jbxvt_get_csi());
    }
}
