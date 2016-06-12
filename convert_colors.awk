#!/usr/bin/gawk -f
BEGIN {
	print "#ifndef COLOR_INDEX_H"
	print "#define COLOR_INDEX_H"
	print "#include <stdint.h>"
	print "static uint8_t color_index[256] = {"
	FS=":";
}

/:/ {
	label = $1
	hex = sprintf("0x%s", $2);
	system("chto " label " " hex);
}

END {
	print "};";
	print "#endif//!COLOR_INDEX_H"
}

