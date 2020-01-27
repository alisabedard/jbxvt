#!/bin/sed -f
1i\
#ifndef JBXVT_COLOR_INDEX_H\n#define JBXVT_COLOR_INDEX_H\n#include <stdint.h>\
static uint32_t jbxvt_color_index[]={
s/\(.*\):\(.*\)/    [\1]=0x\2,/
$a\
\};\n#endif//!JBXVT_COLOR_INDEX_H
/^$/d
s/\(255.*\),/\1/g
