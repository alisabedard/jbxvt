#!/usr/bin/env ruby
# Copyright 2017, Jeffrey E. Bedard
module Generate
	def get_output_file(output_name)
		return File.open output_name, "w"
	end
	def parse_line(out_file, str, &block)
		str = str.split ':'
		if str.length <= 1
			return
		end
		block.call out_file, str
	end
	# Provide a block to generate an output line per input string
	def parse_source(source_name, out_file, &block)
		File.open source_name, "r" do |f|
			f.each_line do |s|
				parse_line out_file, s do | out_file, s |
					block.call out_file, s
				end
			end
		end
	end
end

