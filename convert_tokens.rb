#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
load "header.rb"
load "generate.rb"
include Header, Generate
$serial = 1000
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
def write_header(out_file, tag)
	out_file.write get_guard_top(tag) +
		"enum JBXVTTokenIndex {\n"
end
def parse(source_name, output_name)
	out_file = get_output_file output_name
	tag = get_tag "JBXVTTOKENINDEX"
	write_header out_file, tag
	parse_source source_name, out_file do |out_file, s|
		add_token out_file, s
	end
	write_footer out_file, tag
end
parse "tokens.txt", "JBXVTTokenIndex.h"
