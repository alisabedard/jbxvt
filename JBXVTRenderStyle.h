// Copyright 2017-2020, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTRENDERSTYLE_H
#define JBXVT_JBXVTRENDERSTYLE_H
enum JbxvtRenderStyle {
    JBXVT_RS_BOLD,
    JBXVT_RS_UNDERLINE,
    JBXVT_RS_BLINK,
    JBXVT_RS_RVID,
    JBXVT_RS_ITALIC,
    JBXVT_RS_INVISIBLE,
//  colors:
//  foreground: index or 9 bit octal rgb
    JBXVT_RS_F0,
    JBXVT_RS_F1,
    JBXVT_RS_F2,
    JBXVT_RS_F3,
    JBXVT_RS_F4,
    JBXVT_RS_F5,
    JBXVT_RS_F6,
    JBXVT_RS_F7,
    JBXVT_RS_F8,
//  background: index or 9 bit octal rgb
    JBXVT_RS_B0,
    JBXVT_RS_B1,
    JBXVT_RS_B2,
    JBXVT_RS_B3,
    JBXVT_RS_B4,
    JBXVT_RS_B5,
    JBXVT_RS_B6,
    JBXVT_RS_B7,
    JBXVT_RS_B8,
//  extended color support bits
    JBXVT_RS_CROSSED_OUT,
    JBXVT_RS_DOUBLE_UNDERLINE,
    JBXVT_RS_FG_RGB,
    JBXVT_RS_BG_RGB,
    JBXVT_RS_FG_INDEX,
    JBXVT_RS_BG_INDEX,
    JBXVT_RS_COUNT
};
#endif//!JBXVT_JBXVTRENDERSTYLE_H
