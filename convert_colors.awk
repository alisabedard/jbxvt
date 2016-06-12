#!/usr/bin/gawk -f
BEGIN {
	print "#ifndef COLOR_INDEX_H"
	print "#define COLOR_INDEX_H"
	print "#include <stdint.h>"
	print "static uint32_t color_index[256] = {"
	FS=":";
}

/:/ {
	label = $1
	val = $2
	printf("\t[%d] = 0x%s,\n", label, val);
}

END {
	print "};";
	print "#endif//!COLOR_INDEX_H"
}

