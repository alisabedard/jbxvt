#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
$serial = 1000
def comment(output_file, text)
	output_file.write "\t// #{text}"
end
def add_token(out_file, s)
	out_file.write "\tJBXVT_TOKEN_#{s[0]} "
	# Strip s[1] to get rid of possible new line for comparison
	s[1] = s[1].strip
	if s[1] == "x"
		s[1] = $serial.to_s
		$serial += 1 # set up serial for next call
	end
	out_file.write "= #{s[1]}"
	if s[2] == nil || s[2].length <= 1
		out_file.write ",\n"
	else
		out_file.write ", // #{s[2]}"
	end

end
def get_output_file(output_name)
	return File.open output_name, "w"
end
def write_header(out_file)
	out_file.write "// Copyright 2017, Jeffrey E. Bedard\n" +
		"#ifndef JBXVT_JBXVTTOKENINDEX_H\n" +
		"#define JBXVT_JBXVTTOKENINDEX_H\n" +
		"enum JBXVTTokenIndex {\n"
end
def parse_line(str, out_file)
	str = str.split ':'
	if str.length <= 1
		return
	end
	add_token out_file, str
end
def write_footer(out_file)
	out_file.write "};\n#endif//!JBXVT_JBXVTTOKENINDEX_H\n"
end	
def parse(source_name, output_name)
	out_file = get_output_file output_name
	write_header out_file
	File.open source_name, "r" do |f|
		f.each_line do |s|
			parse_line s, out_file
		end
	end
	write_footer out_file
end
parse "tokens.txt", "JBXVTTokenIndex.h"
