#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
module Header
	def get_include(header)
		return "#include #{header}\n"
	end
	def write_include(output_file, header)
		output_file.write get_include(header)
	end
	def get_comment(text)
		return "// #{text}\n"
	end
	def write_comment(output_file, text)
		output_file.write get_comment(text)
	end
	def get_copyright
		return "Copyright 2017, Jeffrey E. Bedard"
	end
	def get_guard_top(tag)
		return get_comment(get_copyright) +
			"#ifndef #{tag}\n#define #{tag}\n"
	end
	def write_guard_top(out_file, tag)
		out_file.write get_guard_top(tag)
	end
	def get_guard_bottom(tag)
		return "#endif//!#{tag}\n"
	end
	def get_c_block_end
		return "};\n"
	end
	def write_c_block_end(out_file)
		out_file.write get_c_block_end
	end
	def write_guard_bottom(out_file, tag)
		out_file.write get_guard_bottom(tag)
	end
	def get_tag(base)
		return "JBXVT_#{base}_H"
	end
	def write_footer(out_file, tag)
		write_c_block_end out_file
		write_guard_bottom out_file, tag
	end
end
