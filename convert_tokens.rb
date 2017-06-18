#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
$serial = 1000
def comment(output_file, text)
	output_file.write "\t// #{text}"
end
def serialize(s)
	# Strip s[1] to get rid of possible new line for comparison
	s[1] = s[1].strip
	if s[1] == "x"
		s[1] = $serial.to_s
		$serial += 1 # set up serial for next call
	end
end
def add_token(out_file, s)
	out_file.write "\tJBXVT_TOKEN_#{s[0]} "
	serialize s
	out_file.write "= #{s[1]}"
	out_file.write (s[2] == nil || s[2].length <= 1) ?
		",\n" : ", // #{s[2]}"
end
def get_output_file(output_name)
	return File.open output_name, "w"
end
def get_include_guard_header(tag)
	return "// Copyright 2017, Jeffrey E. Bedard\n" +
		"#ifndef #{tag}\n#define #{tag}\n"
end
def write_header(out_file)
	tag = "JBXVT_JBXVTTOKENINDEX_H"
	out_file.write get_include_guard_header(tag) +
		"enum JBXVTTokenIndex {\n"
	return tag
end
def get_include_guard_footer(tag)
	return "};\n#endif//!#{tag}\n"
end
def write_footer(out_file, tag)
	out_file.write get_include_guard_footer tag
end
def parse_line(str, out_file)
	str = str.split ':'
	if str.length <= 1
		return
	end
	add_token out_file, str
end
def parse_source(source_name, out_file)
	File.open source_name, "r" do |f|
		f.each_line do |s|
			parse_line s, out_file
		end
	end
end
def parse(source_name, output_name)
	out_file = get_output_file output_name
	tag = write_header out_file
	parse_source source_name, out_file
	write_footer out_file, tag
end
parse "tokens.txt", "JBXVTTokenIndex.h"
