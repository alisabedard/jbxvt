#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
module Header
	def get_guard_top(tag)
		return "// Copyright 2017, Jeffrey E. Bedard\n" +
			"#ifndef #{tag}\n#define #{tag}\n"
	end
	def get_guard_bottom(tag)
		return "#endif//!#{tag}\n"
	end
	def get_tag(base)
		return "JBXVT_#{base}_H"
	end
end
