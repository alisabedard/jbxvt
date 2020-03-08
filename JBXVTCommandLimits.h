// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVTCOMMANDLIMITS_H
#define JBXVTCOMMANDLIMITS_H
enum JBXVTCommandLimits {
    KEY_MAP_BUF_SIZE = 8,        // size of keyboard mapping buffer
    COM_BUF_SIZE =    UINT8_MAX,    // size of command read buffer
    COM_PUSH_MAX =    20,    // max # chars to put back to input queue
    MP_INTERVAL =    500        // multi-press interval in ms
};
#endif//!JBXVTCOMMANDLIMITS_H

