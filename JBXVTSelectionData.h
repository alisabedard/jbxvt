// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTSELECTIONDATA_H
#define JBXVT_JBXVTSELECTIONDATA_H
#include <stdbool.h>
#include <stdint.h>
#include "JBXVTSelectionUnit.h"
#include "libjb/JBDim.h"
struct JBXVTSelectionData {
    uint8_t * text;
    struct JBDim end[3]; // end0, end1, anchor
    enum JBXVTSelectionUnit unit;
    uint32_t length;
    bool on_screen; /* Large type used for padding
                   to alignment boundary.  */
    const int8_t __pad[3];
};
#endif//!JBXVT_JBXVTSELECTIONDATA_H
