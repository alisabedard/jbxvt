#!/usr/bin/env ruby
# Copyright 2017 Jeffrey E. Bedard
module CaseGenerator
	class Writer
		def initialize(output_file_name = "cases.c")
			@file = File.new output_file_name, "w"
		end
		def write(string)
			@file.write string
		end
	end
	class Coder
		def initialize(_token)
			@token = _token
			set_prefix
		end
		def set_prefix
			@token = "JBXVT_TOKEN_#{@token}"
		end
		def get_break
			return "\tbreak;\n"
		end
		def get_logger
			return "\tLOG(\"#{@token}\");\n"
		end
		def get_fixme
			return "\tLOG(\"FIXME: #{@token}" +
				" not implemented\");\n"
		end
		def get_handler
			return "\tjbxvt_handle_#{@token}(xc, &token);\n"
		end
		def get_case
			return "case #{@token}:\n" 
		end
		def get_logged
			return get_case + get_logger +
				get_handler + get_break
		end
		def get_not_logged
			return get_case + get_handler + get_break
		end
		def get_stub
			return get_case + get_fixme + get_break
		end
	end
	def CaseGenerator.get_tokens
		return %w(ALN CHA CHT CPL CNL CS_ALT_G1 CS_ALT_G2 CS_ALT_G3
		CS_G0 CS_G1 CS_G2 CS_G3 CUB CUD CUF CUP CUU DA DCH DL DSR DWL
		ECH ED EL ELR ENTGM52 EXTGM52 HOME HPA HPR HTS HVP ICH ID IL
		IND LL MC NEL PAM PM PNM RC REQTPARAM RESET RI RIS RQM S7C1T
		S8C1T SAVEPM SBGOTO SBSWITCH SC SD SET SELINSRT SGR SS2 SS3 ST
		STBM SU SWL TBC TXTPAR VPA VPR)
	end
	def CaseGenerator.get_nolog_tokens
		return %w(CHAR STRING)
	end
	def CaseGenerator.get_stubs
		return %w(APC DHLT DHLB EPA OSC SOS SPA)
	end
	def CaseGenerator.main
		w = Writer.new
		get_tokens.each do |token|
			w.write (Coder.new token).get_logged
		end
		get_nolog_tokens.each do |token|
			w.write (Coder.new token).get_not_logged
		end
		get_stubs.each do |token|
			print token + ":s\n"
			w.write (Coder.new token).get_stub
		end
	end
	if __FILE__ == $0
		main
	end
end
