#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
load "generate.rb"
load "header.rb"
include Header, Generate
def add_token(out_file, s)
	out_file.write "\t[#{s[0]}] = 0x#{s[1].strip},\n"
end
def convert_colors(source_name, output_name)
	out_file = get_output_file(output_name)
	tag = get_tag "COLOR_INDEX"
	write_guard_top out_file, tag
	write_include out_file, "<stdint.h>"
	out_file.write "static uint32_t jbxvt_color_index[256] = {\n"
	parse_source source_name, out_file do | out_file, str |
		add_token out_file, str
	end
	write_footer out_file, tag
end
convert_colors "color_index.txt", "color_index.h"
