#!/usr/bin/gawk -f

# Copyright 2016, Jeffrey E. Bedard

# This utility converts colors listed in color_index.txt into the color_index
# array, included with color_index.h
# To re-generate color_index.h, run:
# ./convert_colors.awk color_index.txt > color_index.h

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

