#!/usr/bin/awk -f
# This program serves as a control to verify the behavior of color_index.scm
BEGIN {
	FS=":";
	cr="// Copyright 2017, Jeffrey E. Bedard";
	g="JBXVT_COLOR_INDEX_H";
	print cr "\n#ifndef " g "\n#define " g "\n#include <stdint.h>\n" \
	      "static uint32_t jbxvt_color_index [] = {";
}
/:/ { printf("\t[%d] = 0x%s,\n", $1, $2); }
END { print "};\n#endif//!" g; }
