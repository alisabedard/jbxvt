#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
f=File.open "color_index.txt", "r"
o_name="color_index.h"
#o=File.open o_name, "w"
#File.delete(o_name)
o=File.open o_name, "w"
o.write "// Copyright 2017, Jeffrey E. Bedard\n" +
	"#ifndef JBXVT_COLOR_INDEX_H\n" +
	"#define JBXVT_COLOR_INDEX_H\n" +
	"#include <stdint.h>\n" +
	"static uint32_t jbxvt_color_index[256] = {\n"
while f.gets
	s = $_.split ':'
	if s.length > 1 # ignore blank lines
		s[1]=s[1].strip # delete trailing '\n'
		o.write "\t[#{s[0]}] = 0x#{s[1]},\n"
	end
end
o.write "};\n#endif//!JBXVT_COLOR_INDEX_H\n"
