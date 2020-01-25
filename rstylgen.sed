#!/usr/bin/env sed -f
1i\
// Copyright 2017-2020, Jeffrey E. Bedard\n#ifndef JBXVT_JBXVTRENDERSTYLE_H\
#define JBXVT_JBXVTRENDERSTYLE_H\nenum JbxvtRenderStyle {
s/#/\/\/ /;t;s/^/    JBXVT_RS_/; s/$/,/;s/./\U&/g
$a\
    JBXVT_RS_COUNT\n};\n#endif//!JBXVT_JBXVTRENDERSTYLE_H
